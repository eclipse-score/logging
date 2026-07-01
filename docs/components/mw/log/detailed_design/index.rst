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

.. _component_detailed_design_mw_log:

Detailed Design
###############

.. document:: Logging Component Detailed Design
   :id: doc__mw_logging_detailed_design
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_implementation

This document describes the as-built implementation of :need:`comp__mw_logging`.
Planned changes to the initialization sequence are recorded in :need:`dec_rec__log__explicit_init`
and are not yet reflected here.

Purpose
-------

The ``mw::log`` component provides the logging API consumed by application threads.
Its primary responsibility is to serialize log records into a lock-free shared-memory buffer and
notify the Data Router daemon that data is ready for forwarding to the DLT output backend.
The design minimizes application-thread overhead: no IPC call and no mutex are taken during the
log-record hot path.

Static Design
-------------

The component decomposes into three tiers:

- **Recorder** (``DataRouterRecorder``, ``FileRecorder``, ``TextRecorder``): serializes log
  values from application threads into a slot returned by the backend.
- **Backend** (``DataRouterBackend``, ``SlogBackend``): manages slot allocation using a circular
  allocator. The data-router backend writes serialized records to shared memory; the slog
  backend writes directly to the QNX slog2 buffer.
- **IPC** (message client, shared-memory writer, alternating control block): transfers
  serialized records to the daemon via shared memory with no mutex on the application thread.

The remote-DLT path uses the data-router recorder, data-router backend, and shared-memory
writer. The text recorder wraps the slog backend and is used exclusively on QNX for slog2
output.

On the Linux path, the data-router recorder owns one data-router backend, which owns a message
client. The message client runs the buffer-switch IPC protocol on a background thread and
holds a reference to the shared-memory writer. The shared-memory writer owns the shared-memory
region (alternating control block plus data buffers), created at process startup with a fixed
size from the logging configuration.

The recorder and backend abstract interfaces from ``score_baselibs`` are the component's
extensibility boundary: a new output target requires only a backend implementation (and a
recorder if the serialization differs).

.. uml:: static.puml

Platform Variability
~~~~~~~~~~~~~~~~~~~~

The ``TextRecorder``/``SlogBackend`` backend is selected at build time via Bazel ``select()``
for QNX targets; the ``DataRouterRecorder`` path is the Linux default. Both paths share the
``Recorder`` and ``Backend`` abstract interfaces defined in ``score_baselibs``. No runtime
conditional on platform identity exists in shared code.

Writer/Reader Concurrency Safety
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The alternating control block contains two linear control blocks (even and odd), selected
by an atomic switch counter. Each linear control block tracks the number of active writers,
the total reserved bytes, and the total committed bytes.

Two invariants make concurrent reads safe without a mutex on the writer path:

**Register-before-reserve.** A writer registers in the writer count before reserving any
buffer space. After registering, it validates that the active buffer has not been switched;
a detected switch causes the writer to release its registration on the old block and redirect
to the new one. This ensures the reader cannot observe zero writers while any writer has
reserved but not yet registered — every reserved byte belongs to a registered writer.

**Commit-before-decrement.** A writer's decrement of the writer count is always its last
operation, issued after a release fence and after advancing the committed-bytes counter.
The reader applies an acquire fence after the gate fires, so all payload bytes and the
committed-bytes update are visible before the reader processes the block. A writer that
registers but cannot fit its payload advances the committed-bytes counter to match its
reservation, ensuring the gate's second condition (committed bytes equals reserved bytes)
is always reachable.

The reader's gate requires both conditions simultaneously: zero active writers and
committed bytes equal to reserved bytes.

**Single-reader constraint.** The buffer-switch operation must not be called concurrently.
Buffer switching is invoked by the shared-memory writer's read-acquisition method on the
client's background thread — one thread per session, so the constraint is satisfied by the
client component. The shared-memory writer header states this explicitly: the read-acquisition
method is "thread safe only against allocation and type registration" and "shall not be called
from multiple threads." A code path in the block-acquisition logic — reachable only when
``switch_count`` advances exactly once between the writer's initial read and its registration
— relies on this constraint for its correctness argument. Any evolution to a multi-reader
design requires this code path to be reworked.

All atomics without explicit ordering use the default ``memory_order_seq_cst``.

Dynamic Design
--------------

The hot logging path is entirely wait-free on the application thread.
``StopRecord`` triggers ``FlushSlot``, which calls ``SharedMemoryWriter::AllocAndWrite``.
That function uses ``WaitFreeAlternatingWriter`` to select the active buffer, copy the
serialized ``LogRecord``, and release — no mutex is held at any point on the application thread.

The daemon-side worker thread polls the shared-memory buffer and, when data is ready, sends a
``kAcquireRequest`` message to the client. ``DatarouterMessageClientImpl``'s background thread
handles it in ``OnAcquireRequest()``: calls ``SharedMemoryWriter::ReadAcquire()`` to switch
buffers, then sends ``kAcquireResponse`` back with the ``ReadAcquireResult``. The daemon
stores the result in ``data_acquired`` and finalizes the acquire on the next tick. This
exchange is independent of the application thread's log calls.

.. uml:: dynamic.puml

Wait-Free Justification
~~~~~~~~~~~~~~~~~~~~~~~~

``AllocAndWrite()`` executes a bounded, fixed number of atomic operations on every call path:
no CAS retry loop, no spin. When the buffer is full or the concurrent-writer limit (64) is
reached, ``Acquire()`` returns ``std::nullopt`` immediately — the frame is counted and dropped
without retrying. The ``else if`` branch (switch detected mid-registration) adds one bounded
acquire+release pair; it remains O(1), not a spin.

Buffer-Full and Overflow Behavior
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the active buffer has no remaining capacity, ``Acquire()`` returns ``std::nullopt``.
``AllocAndWrite()`` increments ``SharedData::number_of_drops_buffer_full`` (count) and
``SharedData::size_of_drops_buffer_full`` (bytes) and returns. The daemon reads all three drop counters
(``number_of_drops_buffer_full``, ``number_of_drops_invalid_size``,
``number_of_drops_type_registration_failed``) during each tick and includes them in session
statistics.

Payloads exceeding ``GetMaxPayloadSize()`` (65,500 bytes — the DLT v1 limit) are rejected
before buffer acquisition and counted in ``number_of_drops_invalid_size``.

When ``number_of_writers`` exceeds ``GetMaxNumberOfConcurrentWriters()`` (64), ``Acquire()``
returns immediately and the frame is counted in ``number_of_drops_buffer_full`` — the same
treatment as a full buffer.

The shared-memory region is pre-allocated at process startup with a fixed size from the logging
configuration. Type registration failures (buffer unavailable for the type-registration write)
are retried at the next ``StopRecord`` and counted in
``SharedData::number_of_drops_type_registration_failed``.

Global State and Initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The current implementation initializes the logging backend lazily on first use via a JSON
configuration file. Lazy initialization is a known deviation from the ASIL A+ guideline
restriction on hidden global state: the backend singleton is initialized implicitly
on the first log call rather than at a controlled program point.

This deviation is tracked and planned to be resolved by :need:`dec_rec__log__explicit_init`,
which describes replacing the lazy JSON initialization with an explicit
``score_log_bridge_cpp_init()`` call. Until that change is merged, this deviation is accepted
by the component maintainers as acknowledged technical debt.

Design Decisions
-----------------

**Serialize on the application thread.** Log values are serialized into shared memory by the
recorder, not by the daemon. This keeps the daemon's processing path simple (map raw bytes,
parse, route) and eliminates any serialization-side locking in the daemon.

**Drop-on-overflow, never block.** When the buffer is full, the drop is counted in
``SharedData`` and the application thread continues. Back-pressure on the application thread
is rejected: a slow or crashed daemon must not delay application execution.

**Two-buffer alternating design.** The writer always writes to one ``LinearControlBlock``
while the daemon drains the other. The register-before-reserve + double-gate protocol
ensures no torn reads without requiring any mutex on the writer path.

Security
---------

The ``mw::log`` component writes exclusively into the process's own shared-memory region.
The region is created and mapped by the component itself; no other process has write access
to it. The ``producer_pid`` field in ``SharedData`` is populated at initialization with the
process's own PID so the daemon can cross-check it against the sender of the ``ConnectMessage``.
The component does not read data from the daemon or from other clients' regions.

Exception Behavior
-------------------

The entire hot logging path (``AllocAndWrite()``, ``Acquire()``, ``Release()``) is declared
``noexcept`` and contains no ``throw``/``catch``. The type registration map
(``std::unordered_map``) used during initialization may allocate; that allocation occurs
outside the safety-relevant logging hot path and is bounded by the number of distinct log
message types registered per process.

Library Dependencies
---------------------

.. list-table::
   :widths: 30 50
   :header-rows: 1

   * - Library
     - Role
   * - ``score_baselibs`` v0.2.9
     - ``Recorder``/``Backend`` interfaces, ``CircularAllocator``, ``score::cpp::span``
   * - ``score_communication`` v0.2.1
     - Message-passing client (used by ``DatarouterMessageClientImpl`` for IPC with the daemon)

FuSa/FFI classification of each library is documented in the respective module's safety case.
