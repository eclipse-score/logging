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

DLT Network Transport Evolution
================================

.. dec_rec:: DLT Network Transport Evolution
   :id: dec_rec__logging__dlt_transport_evolution
   :status: proposed
   :version: 1
   :context: See below.
   :decision: TBA

   Today the remote/DLT path is split across two processes, as described in
   :doc:`index` and :doc:`../../../components/datarouter/index`:

   - `mw::log` (application side) serialises log/trace records and writes
     them into a shared-memory buffer.
   - `datarouter` (a separate process) reads that buffer, constructs the
     DLT protocol headers, and transmits the resulting UDP/IPv4 multicast
     packets using the standard BSD Socket API over the platform's default
     network stack, shared with all other networked services.

   A second, feature-flagged client backend (`shm_dma_enabled`)
   would allow forwarding of records through a GTL client into a
   DMA-capable shared-memory region instead of the DataRouter
   ring buffer, handing them to a DLT-aware daemon on the receiving side.
   This is not the default today, and it does not by itself change
   how that receiving daemon talks to the network stack, so the properties
   below still apply regardless of which client backend feeds it.

   This works, but has three structural properties worth revisiting:

   - Every message crosses two IPC hops before it reaches the wire:
     one between `mw::log` and `datarouter`, and a second one from
     `datarouter` into the network stack itself, since the socket API
     it uses is, on this class of platforms, implemented over IPC to a
     separate network-stack process rather than executing inline. Each
     hop also implies copying the message (application buffer into shared
     memory, shared memory into a new buffer with headers prepended, and
     again into the network stack's own send buffers). It is this
     combination of copies and context switches across both hops, not a
     single IPC call, that drives CPU load.
   - Where that network-stack process is a single-threaded resource
     manager (e.g. QNX's `io-pkt`), it serialises *all* socket traffic
     on the system through one queue. This is a separate overhead from
     the copies above: it is contention/scheduling cost, so a burst of
     log and trace traffic can add latency for every other socket user of
     that same instance, and vice versa. Newer, multithreaded stack
     implementations (e.g. `io-sock`) reduce this specific contention,
     but do not by themselves remove the two IPC hops and copies above.
   - log and trace traffic shares the same network stack and send queue as
     other service traffic (E.g. Someip communication), so there is no
     structural isolation between the two; any queuing or scheduling
     behaviour of one can influence the other.

   **Way Forward:**

   Part 1: A GTL-based client backend as the remote-logging path.

   Part 2: Provide a compile-time seam to select between the DLTv1 wire
   format and the DLTv2 wire format, analogous to the existing
   build-flag pattern for Part 1, rather than replacing one with the
   other. Payload serialisation (verbose/non-verbose argument
   encoding) is identical between DLTv1 and DLTv2 and does not need a
   seam. The concrete DLTv2 protocol implementation behind
   this seam is closed-source and maintained outside this repository;
   this repository only needs to own the seam/interface, not the DLTv2
   implementation itself.

   Part 3: Move the DLT header-construction
   and transmission stage (i.e. the network-writing responsibility
   on the daemon receiving GTL records) into a module that is loaded
   directly by the network stack, running on a second, dedicated
   network-stack instance used exclusively for log and trace traffic:

   - Removes the second IPC hop (and its associated copy) between the
     router logic and the network stack for the transmit path.
   - Allows direct use of the network stack's native buffer/interface APIs
     instead of the generic socket API, removing at least one further
     copy and enabling zero-copy transmission where supported by the
     driver.
   - Structurally isolates log and trace traffic from other network traffic,
     since it no longer shares a network-stack instance, queue, or
     scheduling domain with it.
   - Enables transport-level controls (e.g. egress traffic shaping) to be
     applied specifically to the log and trace traffic instance without
     affecting other traffic.

   .. uml:: _assets/lsm_plugin.puml

   Out of scope / unaffected:

   - The `mw::log` application-facing logging APIs are unaffected; only
     the backend/transport selected underneath it changes.
   - The DLT payload serialisation (verbose/non-verbose argument encoding)
     is unaffected, since it is identical between DLTv1 and DLTv2.
   - The DLTv2 protocol implementation is out of scope: this repository only
     provides the compile-time seam to select it (Part 2); the
     implementation behind that seam is closed-source and lives in a
     separate, non-public `repository <https://github.com/comasso>`_.
   - Freedom-from-interference (FFI) guarantees is unaffected and already
     provided by the existing mw::log infrastructure.

   **Trade-offs**

   - Both halves of this evolution targets DMA/zero-copy where the
     target hardware happens to support it. Each target platform
     needs to be verified and configured individually; where
     DMA/zero-copy isn't available, the transport still works,
     just without the associated performance benefit.
   - Introduces a second network-stack instance that must be configured,
     operated, and kept isolated from the default one.
   - Requires a feasibility phase to confirm the target network stack
     supports loadable modules with the required capabilities for the
     supported target platforms.
