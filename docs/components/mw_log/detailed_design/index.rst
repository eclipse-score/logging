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


mw::log Backend Detailed Design
###############################

.. document:: mw::log Backend Detailed Design
   :id: doc__mw_log_backend_detailed_design
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_implementation

Description
===========

The ``mw::log`` backend selects a recorder at runtime (see
:need:`comp_arc_sta__log__sv`). The recorders and backends are shown in the
static diagram below.

The design documents and PlantUML diagrams are part of this folder (module
template layout). The ``architectural_design`` target, which validates the
diagrams, is in the root ``BUILD``.

Design Decisions
----------------

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Decision
     - Rationale
   * - Recorder and backend are separate abstractions (``Recorder`` formats and
       copies the data, ``Backend`` provides and flushes slots).
     - The same slot handling is reused by the file, syslog, slog and datarouter
       outputs. A new output only needs a new ``Backend``.
   * - Backends are decoupled from the log record format: the recorder factory
       passes an ``IMessageBuilder`` to the backend.
     - The backend stays independent of ``LogRecord`` details and can be tested
       with a mock message builder.
   * - Zero-copy slots from a ``CircularAllocator``. The backend only handles
       ``SlotHandle`` objects.
     - No copy of the data between recorder and backend. Memory is allocated once
       at construction (:need:`comp_req__log__local_allocation_strategy`).
   * - Backends are non-blocking. If all slots are in use, no slot is returned
       and existing data is not overwritten.
     - A logging call must not block or corrupt data of the caller
       (:need:`comp_req__log__avoid_locks`).
   * - OS functions (``Fcntl``, ``Unistd``, ``Syslog``, ``Slog2``) are injected
       through constructor arguments.
     - Unit tests on the host without a running syslog daemon or real files.
   * - Datarouter backend: log data goes through shared memory, the message
       passing connection only synchronizes access. The client is passive.
     - Lock-free and wait-free logging in the client, and freedom from
       interference with the not safety-qualified Datarouter. Details in
       :doc:`datarouter_backend/index`.
   * - The client unlinks the shared memory file after the first message from
       Datarouter and on exit.
     - The file can only be removed by its owner. This avoids leaked files.
   * - Each backend is registered for a ``LogMode`` in ``backend/*_registrant.cpp``.
     - The backends are selected at runtime by configuration, see
       :need:`comp_arc_sta__log__sv`.

Design decisions that need a formal record use :need:`gd_temp__change_decision_record`;
see also :need:`dec_rec__logging__explicit_init`.

Design Constraints
------------------

- Local allocators are used instead of the global heap; global heap allocation is
  limited to the initialization phase (:need:`comp_req__log__local_allocation_strategy`).
- Loops are bounded (:need:`comp_req__log__no_endless_loops`): ``CircularAllocator``
  limits the search for a free slot, ``SlotDrainer`` limits the slots per flush
  cycle.
- The number of slots is limited by the type of ``SlotIndex`` (``std::uint8_t``).
- The payload of one entry in the shared memory ring buffer is limited to 65500
  bytes (``SharedMemoryWriter::GetMaxPayloadSize()``). Larger entries are dropped
  and counted.
- A message passing message has at most 17 bytes, a notification 1 byte
  (``MessagePassingConfig``).
- Input from Datarouter is untrusted: unexpected notifications are ignored.
- The syslog backend is available on Linux only, the slog backend on QNX only.
- The file output backend is not meant for production use. It uses a mutex in
  ``SlotDrainer`` and is disabled in production.
- The dynamic identifier mode of the datarouter backend is for testing only.
- All requirements of the component apply, see :need:`doc__mw_log_backend_requirements`.

External Dependencies
---------------------

The dependencies were collected from the Bazel build graph (direct dependencies
of the non-test ``cc_library`` targets of each unit outside of ``//score/mw/log``):

.. code-block:: bash

   bazel query "labels(deps, kind('cc_library', //score/mw/log/detail/<unit>:*)
     except attr(testonly, 1, //score/mw/log/detail/<unit>:*)) except //score/mw/log/..."

The safety rating is taken from the ``comp`` need of the providing component.

.. list-table::
   :header-rows: 1
   :widths: 30 35 20 15

   * - Dependency
     - Used by
     - Component
     - Rating
   * - ``score_baselibs`` ``score/language/futurecpp``
     - all units
     - futurecpp
     - ``asil-b`` tag in the build file
   * - ``score_baselibs`` ``score/mw/log`` (``minimal``, ``recorder``,
       ``shared_types``, ``configuration``, ``detail`` interfaces and data types,
       ``text_recorder``)
     - all units
     - ``comp__log``
     - ASIL_B
   * - ``score_baselibs`` ``score/os`` (``fcntl``, ``mman``, ``stat``, ``stdlib``,
       ``pthread``, ``unistd``, ``syslog``, ``qnx/slog2``, ``utils/signal``,
       ``utils/high_resolution_steady_clock``)
     - syslog, slog, datarouter, file output backend
     - ``comp__baselibs_os``
     - ASIL_B
   * - ``score_baselibs`` ``score/concurrency``
     - datarouter backend
     - ``comp__baselibs_concurrency``
     - ASIL_B
   * - ``score_baselibs`` ``score/static_reflection_with_serialization``
     - file output backend (DLT message builder)
     - ``comp__baselibs_static_reflection``
     - ASIL_B
   * - ``score_communication`` ``score/message_passing``
     - datarouter backend
     - Communication
     - not stated in the component docs
   * - ``score_communication`` ``score/memory/shared``
     - datarouter backend
     - Communication
     - not stated in the component docs

The libraries ``boost`` and ``nlohmann_json`` are only used transitively through
``score_baselibs`` and ``score_communication``. No unit depends on them directly.

``score_baselibs`` ``score/os/utils:high_resolution_steady_clock`` is marked as
deprecated by its provider (replacement ``HighPrecisionLocalSteadyClock``). It is
used by ``detail/data_router/shared_memory:common`` and
``detail/file_recorder:file_recorder``.

Rationale Behind Decomposition into Units
-----------------------------------------

The component is split by output channel so that each unit has a single
responsibility and can be tested and selected independently:

- file output backend: provides slots and writes records through a sink (file output)
- syslog backend: forwards records to the system logger
- slog backend: forwards records to the QNX system logger (slog2)
- datarouter backend: writes records to shared memory for the datarouter
- backend registration: connects the backends to the log modes without a
  dependency of the frontend on the backends

Static Diagrams for Unit Interactions
=====================================

The backend composition and recorder relationships are shown below:

.. uml:: _assets/mw_log_recorders.puml

Further static diagrams are part of the unit documents listed under
`Units within the Component`_.

Dynamic Diagrams for Unit Interactions
======================================

The dynamic views (sequence diagrams) are part of the unit documents listed under
`Units within the Component`_.

Units within the Component
==========================

The decomposition into units is given by the directory and file structure; a unit
belongs to this component through its location. Attributes, behavior and the
interfaces of a unit are documented in the source code (public headers). Paths are
relative to ``score/mw/log`` of this repository unless noted otherwise.

.. list-table::
   :header-rows: 1
   :widths: 20 45 35

   * - Unit
     - Classes and source code
     - Design
   * - File output backend
     - ``FileRecorder``, ``FileRecorderFactory``, ``DltMessageBuilder`` in
       ``detail/file_recorder``;
       ``FileOutputBackend``, ``SlotDrainer``, ``NonBlockingWriter``,
       ``IMessageBuilder`` in ``score_baselibs`` ``score/mw/log/detail/text_recorder``
     - :doc:`file_output_backend`
   * - Syslog backend
     - ``SyslogBackend``, ``SyslogRecorderFactory`` in ``detail/syslog``
     - :doc:`syslog_backend`
   * - Datarouter backend
     - ``DataRouterBackend``, ``DataRouterRecorder``, ``DatarouterMessageClient``,
       ``DatarouterMessageClientImpl`` and factories in ``detail/data_router``;
       ``SharedMemoryWriter``, ``SharedMemoryReader``, ``SharedData``,
       ``BufferEntryHeader`` in ``detail/data_router/shared_memory``
     - :doc:`datarouter_backend/index`
   * - Slog backend (QNX)
     - ``SlogBackend``, ``SlogRecorderFactory`` in ``detail/slog``
     - :doc:`slog_backend`
   * - Wait-free producer queue
     - ``detail/wait_free_producer_queue``
     - README in the unit directory
   * - Backend registration
     - ``backend/*_registrant.cpp``
     - :doc:`backend_registration`

Interfaces of the Component
---------------------------

The interfaces of the component architecture are realized by the following
source code interfaces. The interface documentation is part of the headers.

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Architecture interface
     - Source code interface
     - Realizing units
   * - ``logic_arc_int__log_cpp__logging``
       (operation ``logic_arc_int_op__logging__log``)
     - ``mw::log::Recorder`` (``score_baselibs`` ``score/mw/log/recorder.h``)
     - ``DataRouterRecorder``, ``FileRecorder``; ``TextRecorder`` with
       ``SyslogBackend`` or ``SlogBackend``
   * - :need:`logic_arc_int__logging__shm`
     - ``SharedMemoryWriter`` (writer side), ``ISharedMemoryReader`` (reader side),
       data layout ``SharedData`` and ``BufferEntryHeader``
     - Datarouter backend
   * - :need:`logic_arc_int__logging__session_ctrl_channel`
     - ``DatarouterMessageClient`` and the messages in ``data_router_messages.h``
     - Datarouter backend

The internal interface ``mw::log::detail::Backend`` (``ReserveSlot()``,
``FlushSlot()``) is implemented by ``FileOutputBackend``, ``SyslogBackend``,
``SlogBackend`` and ``DataRouterBackend``.

.. toctree::
   :maxdepth: 1

   file_output_backend
   syslog_backend
   slog_backend
   backend_registration
   datarouter_backend/index


Inspection Checklist
====================

The checklist for verification of the detailed design and code can be found here:

.. toctree::

   chklst_impl_inspection
