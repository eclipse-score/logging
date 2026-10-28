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

The ``mw::log`` backends are selected in two steps (see
:need:`comp_arc_sta__log__sv`). At build time, the application links the
backends it needs. Each linked backend registers itself for a ``LogMode``
before ``main()`` (``backend/*_registrant.cpp``). At runtime, the logging
application configuration selects the ``LogMode`` values and a recorder is
created for each of them that has a registered backend. The recorders and
backends are shown in the static diagram below.

The design documents and PlantUML diagrams are in
``score/mw/log/design/backend``. This folder links them by symbolic links, so
that the files stay at their place in the source tree. The
``architectural_design`` target ``//score/mw/log/design/backend:mw_log_backend_architectural_design``
validates the diagrams.

Design Decisions
----------------

The diagrams show how the design addresses the requirements and concerns.

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Requirement or concern
     - Diagram
   * - New outputs only need a new backend: recorder and backend are separate
       abstractions
     - ``mw_log_recorders.puml`` in `Static Diagrams for Unit Interactions`_
   * - Backends are independent of the log record format
     - ``mw_log_file_backend.puml`` in :doc:`file_output_backend`
   * - Local allocation of the log slots
       (:need:`comp_req__log__local_allocation_strategy`)
     - ``mw_log_file_backend.puml`` in :doc:`file_output_backend`
   * - A logging call does not block (:need:`comp_req__log__avoid_locks`)
     - ``mw_log_file_backend.puml`` in :doc:`file_output_backend`
   * - Unit tests without real OS resources
     - ``mw_log_file_backend.puml`` in :doc:`file_output_backend` and
       ``datarouter_class_diagram.puml`` in :doc:`datarouter_backend/index`
   * - Lock-free logging and freedom from interference with the Datarouter
     - ``inter_process_communication.puml`` and
       ``verbose_logging_sequence.puml`` in :doc:`datarouter_backend/index`
   * - No leaked shared memory file
     - ``datarouter_message_client_impl_connecttodatarouter.puml`` in
       :doc:`datarouter_backend/index`
   * - The frontend does not depend on the backends
     - ``backend_registration_sequence_diagram.puml`` in ``score_baselibs``
       (``score/mw/log/design``)

Design decisions that need a formal record use :need:`gd_temp__change_decision_record`;
see also :need:`dec_rec__logging__explicit_init`.

Design Constraints
------------------

All requirements of the component apply, see :need:`doc__mw_log_backend_requirements`.
The diagrams show the constraints per building block.

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Building block
     - Constraint
     - Diagram
   * - All units
     - Local allocators instead of the global heap, which is limited to the
       initialization phase (:need:`comp_req__log__local_allocation_strategy`)
     - ``mw_log_file_backend.puml`` in :doc:`file_output_backend`
   * - Slot handling
     - Bounded loops
     - ``mw_log_file_backend.puml`` in :doc:`file_output_backend`
   * - File output backend
     - Not for production use, it uses a mutex in ``SlotDrainer``
     - ``mw_log_file_backend.puml`` in :doc:`file_output_backend`
   * - Syslog backend, slog backend
     - Syslog on Linux only, slog on QNX only
     - ``mw_log_recorders.puml`` in `Static Diagrams for Unit Interactions`_
   * - Datarouter backend
     - Size limit of a shared memory entry, larger entries are dropped and
       counted
     - ``shared_memory_writer_allocandwrite.puml`` in
       :doc:`datarouter_backend/index`
   * - Datarouter backend
     - Size limit of messages and notifications, input from the Datarouter is
       untrusted
     - ``inter_process_communication.puml`` in :doc:`datarouter_backend/index`
   * - Datarouter backend
     - Dynamic identifier mode for testing only
     - ``write_factory_design.puml`` in :doc:`datarouter_backend/index`

External Dependencies
---------------------

External dependencies and their relationships are defined in the Bazel BUILD
targets.
FFI classifications are maintained using Bazel target metadata,
including the ``FFI`` tag.
The dependency tree used for implementation inspection and
safety analysis is generated from the Bazel dependency graph.

Rationale Behind Decomposition into Units
-----------------------------------------

The component is split by output channel so that each unit has a single
responsibility and can be tested and selected independently. The units and
their relations are shown in ``mw_log_recorders.puml`` in
`Static Diagrams for Unit Interactions`_.

Static Diagrams for Unit Interactions
=====================================

The backend composition and recorder relationships are shown below:

.. uml:: _assets/mw_log_recorders.puml

Further static diagrams are part of the unit documents listed under
`Units within the Component`_.

Dynamic Diagrams for Unit Interactions
======================================

The sequence of one log call through the syslog backend is shown below.
Further dynamic views (sequence diagrams) are part of the unit documents listed
under `Units within the Component`_.

.. uml:: _assets/syslog_backend_sequence.puml

Units within the Component
==========================

The decomposition into units is given by the directory and file structure; a unit
belongs to this component through its location. Attributes, behavior and the
interfaces of a unit are documented in the source code (public headers). Paths are
relative to ``score/mw/log`` of this repository unless noted otherwise.

Interfaces of the Component
---------------------------

The architecture interfaces are realized by the source code interfaces shown in
the diagrams. The interface documentation is part of the headers.

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Architecture interface
     - Diagram
   * - :need:`logic_arc_int__log_cpp__logging`
     - ``mw_log_recorders.puml`` in `Static Diagrams for Unit Interactions`_
   * - :need:`logic_arc_int__logging__shm`
     - ``datarouter_class_diagram.puml`` in :doc:`datarouter_backend/index`
   * - :need:`logic_arc_int__logging__session_ctrl_channel`
     - ``datarouter_class_diagram.puml`` in :doc:`datarouter_backend/index`

.. toctree::
   :maxdepth: 1

   file_output_backend
   datarouter_backend/index


Inspection Checklist
====================

The checklist for verification of the detailed design and code can be found here:

.. toctree::

   chklst_impl_inspection
