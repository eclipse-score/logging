..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

.. _component_detailed_design_datarouter:

Detailed Design
###############

.. document:: Data Router Detailed Design
   :id: doc__data_router_detailed_design
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_implementation

This document describes the as-built implementation of :need:`comp__data_router`.

Purpose
-------

The Data Router daemon receives log records from client processes via shared memory and routes
them to the DLT output backend (UDP multicast, file, or both). It is the only process that holds
a DLT connection; the client-side ``mw::log`` library writes to shared memory without contacting
the daemon on the hot path. The daemon also applies per-context and per-application log-level
filters before forwarding.

Static Design
-------------

The Data Router daemon consists of three main subsystems:

``SocketServer`` provides the daemon's entry point and coordinates subsystem initialization.
It constructs the DLT output handler, creates the main data routing component, and starts
the message-passing server that accepts client connections.

``DataRouter`` maintains the routing table and manages active client sessions. Each connected
client has an associated session that parses log records from that client's shared-memory region
and forwards them to the DLT output handler. The data router tracks all active sessions for
statistics reporting and broadcast operations.

``MessagePassingServer`` runs two threads: a dispatch thread that accepts new client connections,
and a worker thread that processes log data from shared memory. This separation prevents slow
shared-memory reads from blocking new connection requests. When a client connects, the dispatch
thread creates a session and hands it to the worker thread. The worker thread periodically reads
from each client's shared memory, parses log records, and routes them through the filtering and
output pipeline.

``DltLogServer`` implements the output pipeline for verbose, non-verbose, and file-transfer
log messages. Each message type has its own output interface. The log parser dispatches to
the appropriate interface based on the message type, and the DLT server applies per-context
filtering before forwarding to the DLT transmission layer.

.. uml:: static.puml

Session Registry Concurrency
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The dispatch thread and worker thread both access the session registry. The message-passing
server protects the session list with a mutex. When a new connection arrives, the dispatch
thread locks the mutex, adds the session to the registry, and unlocks. The worker thread
acquires the same mutex during its tick loop to safely iterate over active sessions.

The data router maintains its own session tracker for statistics and broadcast operations.
This tracker is protected by a separate mutex. When creating or destroying sessions, both
mutexes must be acquired to maintain consistency between the message-passing server's registry
and the data router's tracking set.

Per-session mutable state (subscriber configuration, command interfaces, statistics) is
protected by per-session mutexes, allowing the worker thread to read log data from a session
while the dispatch thread modifies that session's configuration.

Thread shutdown uses the stop-token mechanism. On destruction, the stop token is set and the
worker thread is joined before any session state is freed, preventing use-after-free on the
session registry.

Filter reconfiguration acquires a configuration mutex before modifying the filter table.
The filter evaluation path holds the same mutex for the duration of the filter check, ensuring
consistent reads and writes at runtime.

Input Validation
~~~~~~~~~~~~~~~~~

Each connection request carries the application ID, UID, and a random session identifier.
The client process ID is supplied by the QNX message-passing transport layer, not by the
message payload. During session creation, the daemon cross-checks this transport-provided PID
against the PID written by the client library in the shared-memory header. A mismatch causes
the daemon to log a warning and reject the session — no shared-memory reader is created and
the client's logs are not forwarded.

The shared-memory file name is derived from the connection request fields before the file
is opened. The daemon opens the file itself with read-only mapping, so the client cannot
redirect the mapping to an arbitrary path after sending the connection request.

The log parser registers type descriptors by type identifier. During parsing, an unknown
type identifier that is not present in the parser's registry causes the entry to be silently
dropped; no exception is raised. Each log parser is owned by a single session and accessed
only from the worker thread that owns that session.

The buffer-switch protocol splits responsibility between client and daemon. The client's
background thread requests a buffer switch and sends the result to the daemon in an
acquisition-response message. The daemon validates that all writers have released the buffer
(checking that the number of active writers is zero and the write index matches the acquired
index) before allowing the reader to access the newly-acquired buffer. This prevents the
daemon from reading a block while any writer is still mid-commit. Full concurrency invariants
are documented in :ref:`component_detailed_design_mw_log` (Writer/Reader Concurrency Safety
section).

Configuration
~~~~~~~~~~~~~

The daemon loads configuration from two sources.

``SocketServer::CreateDltServer()`` reads ``./etc/log-channels.json`` (relative to the working
directory) via ``ReadStaticDlt()`` to configure DLT channel assignments — which DLT
application/context identifiers map to which output channels. If this file is missing or
malformed, ``CreateDltServer()`` logs an error and returns null; the daemon aborts without a
valid DLT server.

``SocketServer::LoadNvConfig()`` reads per-context log-level thresholds from the path given by
its second argument (default: ``/bmw/platform/opt/datarouter/etc/class-id.json``). These
thresholds are used by ``FilterAndCall()`` to filter log entries per context. If missing or
malformed, ``LoadNvConfig()`` logs a warning and returns an empty config; the daemon continues
without per-context filtering.

The client-side shared-memory buffer size is determined by the logging configuration supplied
to the client library at initialization time (lazy JSON loading — see Global State deviation
in :ref:`component_detailed_design_mw_log`). The buffer size affects how many log records can
be in-flight before drops occur; it is not controlled by the daemon.

Dynamic Design
--------------

Session establishment begins when a client sends a ``kConnect`` message containing the
application ID, UID, and random identifier. The dispatch thread creates a new session with its
own log parser and shared-memory reader. The message-passing server takes ownership of the
session, while the data router maintains a tracking reference for statistics and broadcast
operations.

After connection, the application thread writes log data directly to shared memory (wait-free,
no daemon involvement). The daemon initiates buffer switching by sending a ``kAcquireRequest``
message to the client. The client performs the buffer switch and replies with a
``kAcquireResponse`` message carrying the acquisition result. The dispatch thread stores this
result and enqueues the session for processing. The worker thread then processes the session
in two phases: Phase 1 validates that all writers have released the buffer (checking the gate
conditions described in :ref:`component_detailed_design_mw_log`), then configures the reader
for the newly-acquired buffer. Phase 2 reads log records from shared memory and invokes
per-type callbacks. These callbacks parse each record and dispatch to the DLT output handler
via the registered output interfaces, which apply context filters before forwarding to the
DLT subsystem.

.. uml:: dynamic.puml

Client-Crash and Session Lifecycle
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Both a clean client disconnect and an unclean disconnect (client crash) are detected through
the same mechanism: the QNX message-passing channel fires a peer-close event when the client
process terminates or closes its connection. This event triggers the session teardown sequence.
The dispatch thread marks the session for deletion and enqueues it for final processing by the
worker thread.

The worker thread detects the peer-close condition and performs a final read from the session's
shared-memory region to drain any remaining log records. On a clean disconnect, the client
library has signaled completion before closing the channel; on a crash, this signal is absent
but the daemon still attempts to read any committed records. After draining, the worker removes
the session, unmaps the shared-memory region, and frees the log parser.

If a new connection request arrives from the same process ID while an existing session is still
active (rapid client restart), the daemon finalizes the old session before creating the new one.

The daemon also implements a watchdog counter that tracks worker iterations during which a
client sends no buffer-switch acknowledgment. The channel-close event is the primary detection
mechanism for crashes; the watchdog only triggers teardown when that event is delayed or lost.
A client that logs rarely stays active as long as the channel is open; the watchdog threshold
is set higher than any expected quiet period.

Error Paths
~~~~~~~~~~~

- **Buffer full (writer side)**: The client library's allocation function increments a drop
  counter and returns without writing. The daemon reads all three drop counters (buffer full,
  invalid size, type registration failed) during session processing and logs them as session
  statistics.
- **Quota exceeded**: When quota overlimit is detected, the session's record-processing callback
  returns immediately for each incoming record without parsing or forwarding to DLT output.
  These records are silently skipped and not counted in the shared-memory drop counters or
  session statistics.
- **Unknown type identifier during parsing**: The entry is silently dropped; the session
  continues processing subsequent records.
- **Malformed payload (known type, invalid length or overrun)**: The daemon reads only up to
  the committed-bytes boundary from the shared-memory region. The linear reader is bounded to
  this committed range and does not read beyond it, regardless of any field values in the
  payload.
- **DLT send failure (UDP unavailable, disk full)**: Handled by the underlying DLT library.
  The daemon does not apply additional counters or retry logic on DLT-layer send failures;
  failures are observable through the DLT library's own diagnostics.
- **Duplicate connection request for an existing process ID**: The old session is finalized
  synchronously before the new one is created.
- **Process ID mismatch during session creation**: Session creation is rejected and the channel
  connection is closed.
- **Stale session (watchdog counter exceeded)**: Session is marked for teardown on the next
  worker iteration (backstop mechanism only — see above).

Memory Bounds
--------------

Per session, the daemon allocates one log parser (handler maps sized by the number of
registered log types for that client) and one shared-memory reader (maps the client's
pre-allocated, fixed-size shared-memory region). The shared-memory region is fixed in size at
client startup; the daemon maps it read-only and unmaps it on session teardown.

The data router's session tracker has no configured upper bound on session count. In the
current integration, the logging clients are a fixed, trusted set of platform processes, and
the OS file-descriptor limit acts as an implicit bound. A configurable maximum session count
is not yet implemented; this is a known gap without a separate tracking issue at the time of
writing. It is acceptable because the client population is controlled by the integrator,
not by untrusted external input.

Per-session type map growth (the parser's type registry) is also client-driven: a client
registering many distinct log message types grows the daemon's per-session heap without an
explicit cap. The same reasoning applies; in practice, the type count is bounded by the
number of distinct log call sites compiled into the client binary.

Security
--------

- **PID verification**: The daemon cross-checks the sender process ID from the QNX
  message-passing layer against the process ID the client library stored in the shared-memory
  header at initialization. A mismatch causes session creation to fail — the client's logs are
  not forwarded and no session with a valid reader can be established.
- **Shared-memory file access control**: The shared-memory file is opened by the daemon after
  path resolution. OS file permissions restrict which processes can create a file with a given
  name. The daemon maps the file read-only.
- **DLT context filtering**: Per-context and per-application log-level filters are applied
  (under mutex protection) before forwarding. A compromised client cannot bypass filtering by
  writing directly to shared memory — filtering occurs on the daemon-owned path.
- **No cross-session state leakage**: Session finalization ensures a restarting client with the
  same process ID does not inherit type registrations from its prior session.
- **Payload bounds**: The daemon reads only up to the committed-bytes boundary per block,
  regardless of field values inside the payload. Client-controlled field values (e.g., a length
  field claiming more bytes than committed) cannot cause the reader to access memory outside
  the shared-memory mapping.
- **Per-session type map growth**: Noted as an unbounded client-driven allocation in the Memory
  Bounds section; trust-boundary rationale applies.

Exception Behavior
-------------------

The client-side hot path (allocation, acquisition, release operations) is declared noexcept.
On the daemon side, the log parser's type lookup uses map queries that do not throw. Type
registration inserts into the map and may trigger allocation; this occurs outside the
log-forwarding hot path. The daemon's threads are managed with standard thread wrappers; an
unhandled exception on a thread propagates to the thread boundary and results in process
termination. Process termination is the defined safe state for this daemon: the platform's
health monitor (PHM) detects the missing heartbeat and triggers the system reaction defined
in the platform safety concept.

Library Dependencies
---------------------

.. list-table::
   :widths: 30 50
   :header-rows: 1

   * - Library
     - Role
   * - ``score_baselibs`` v0.2.9
     - ``AlternatingControlBlock``, ``LinearControlBlock``, ``span``, ``Synchronized<T>``
   * - ``score_communication`` v0.2.1
     - Message-passing server/client (``IServerFactory``, ``IClientFactory``, ``IServer``)

FuSa/FFI classification of each library is documented in the respective module's safety case.

Design Decisions
-----------------

**Shared memory for bulk transfer, message-passing only for buffer-switch coordination.** Bulk
log data travels via the shared-memory alternating buffer (wait-free, no daemon involvement on
the write path). The daemon initiates buffer-switch cycles by sending a request to the client;
the client performs the buffer switch and replies with an acknowledgment. This decouples
log-record throughput from IPC latency: the client can commit many records between buffer-switch
cycles.

**Three output interfaces on DltLogServer.** Implementing all three interfaces directly on the
DLT log server eliminates a branch in the routing path and makes each message type's output
path independently testable.

**Dispatch/worker thread split in MessagePassingServer.** Separating connection acceptance from
shared-memory reads prevents a slow or stalled client from blocking new connection requests.
The work queue decouples the two threads.

**Per-session log parser.** Each session owns its log parser and its type registry. This avoids
shared mutable state across sessions, makes session teardown deterministic (destroy the session,
destroy the parser), and enforces the single-writer invariant on the type registry without
a lock.
