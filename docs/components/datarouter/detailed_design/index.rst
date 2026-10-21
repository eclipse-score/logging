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

Detailed Design
###############

.. document:: Datarouter Detailed Design
   :id: doc__datarouter_detailed_design
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__sw_implementation[version==1]

Description
===========

The Datarouter (:need:`comp__datarouter`) is the DLT daemon. It reads the log
records of the clients from shared memory, filters them, and sends them as DLT
messages over UDP multicast. A client (``mw::log``) connects through a message
passing channel. The Datarouter asks the client to hand over a buffer by a
notification and reads it from the shared memory. It never blocks on a client.

The design documents and PlantUML diagrams are in ``score/datarouter/doc/design``.
This folder links them by symbolic links, so that the files stay at their place in
the source tree. The source code is in ``score/datarouter``. Paths in this document
are relative to this directory unless noted otherwise.

Design Decisions
----------------

The diagrams show how the design addresses the requirements and concerns.

.. list-table::
   :header-rows: 1
   :widths: 45 55

   * - Requirement or concern
     - Diagram
   * - Freedom from interference: the Datarouter is a separate process and only
       reads the shared memory of the clients
     - ``detail-ipc.puml`` and ``context-highlevel.puml`` in
       :doc:`logging_architecture`
   * - A client that does not answer must not stall the other clients
     - ``client_session_interaction_sequence.puml`` in
       `Dynamic Diagrams for Unit Interactions`_
   * - Stale sessions are cleaned up without losing readable data
     - ``activity_watchdog_session_lifecycle.puml`` in
       :doc:`logging_architecture`
   * - A client that crashes while its session is created
     - ``sequence_early_disconnect_race.puml`` in :doc:`logging_architecture`
   * - Reduce the traffic early, own level per channel
     - ``dlt_message_filtering_backend.puml`` in :doc:`logging_architecture`
   * - Bandwidth quota per source
       (:need:`comp_req__data_router__dlt_quota_bw_reduction`)
     - No diagram shows the quota, see IMPL_01_03 in
       :doc:`chklst_impl_inspection`
   * - One code base for different deployments, stubs keep the interfaces
     - ``file_transfer_feature_class_diagram.puml`` in
       :doc:`logging_architecture` and ``package-datarouter.puml`` in
       `Static Diagrams for Unit Interactions`_
   * - Deployment-specific configuration without a rebuild
       (:need:`comp_req__data_router__dlt_multiple_channels`)
     - ``dlt_message_filtering_backend.puml`` in :doc:`logging_architecture` and
       ``seq_kvs_threads_sequence_diagram.puml`` in
       `Dynamic Diagrams for Unit Interactions`_
   * - The persistent storage is provided by the platform
     - ``seq_kvs_threads_sequence_diagram.puml`` in
       `Dynamic Diagrams for Unit Interactions`_

Design Constraints
------------------

All requirements of the component apply, see :need:`doc__data_router_requirements`.
The diagrams show the constraints per building block.

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Building block
     - Constraint
     - Diagram
   * - DLT output
     - UDP multicast only (:need:`comp_req__data_router__dlt_server`)
     - ``context-highlevel.puml`` in :doc:`logging_architecture`
   * - Configuration
     - ``log-channels.json`` is mandatory, ``class-id.json`` is optional
     - ``dlt_message_filtering_backend.puml`` in :doc:`logging_architecture`
   * - Sessions
     - One worker thread serves all sessions, the notification is non-blocking
     - ``client_session_interaction_sequence.puml`` in
       `Dynamic Diagrams for Unit Interactions`_
   * - Watchdog
     - A session is removed after a limited number of missed acquire deadlines
     - ``activity_watchdog_session_lifecycle.puml`` in
       :doc:`logging_architecture`
   * - Configuration sessions
     - Changes are not persistent in this repository (stub dictionary)
     - ``seq_kvs_threads_sequence_diagram.puml`` in
       `Dynamic Diagrams for Unit Interactions`_

External Dependencies
---------------------

External dependencies and their relationships are defined in the Bazel BUILD
targets. The dependency analysis and its findings are part of the checklist item
IMPL_01_04 in :doc:`chklst_impl_inspection`.

Rationale Behind Decomposition into Units
-----------------------------------------

The component is split by responsibility so that each unit can be tested and
replaced independently. The units and their relations are shown in
``datarouter_units.puml`` and ``package-datarouter.puml`` in
`Static Diagrams for Unit Interactions`_.

Static Diagrams for Unit Interactions
=====================================

.. uml:: _assets/package-datarouter.puml

.. uml:: _assets/context-sw-units.puml

The units that appear in the sequence diagrams and their bindings:

.. uml:: _assets/datarouter_units.puml

The diagrams are validated by the ``architectural_design`` target
``//score/datarouter/doc/design:datarouter_architectural_design``. The interface
names in the unit diagram follow the convention of the validator (``I<caller><callee>``)
and are not interfaces of the code.

Further static diagrams are part of :doc:`logging_architecture`.

Dynamic Diagrams for Unit Interactions
======================================

The session handling between a client and the Datarouter:

.. uml:: _assets/client_session_interaction_sequence.puml

The configuration session:

.. uml:: _assets/seq_kvs_threads_sequence_diagram.puml

Further dynamic diagrams (early disconnect, watchdog, filtering) are part of
:doc:`logging_architecture`.

Units within the Component
==========================

The decomposition into units is given by the directory and file structure; a unit
belongs to this component through its location. Attributes, behavior and the
interfaces of a unit are documented in the source code (public headers). Paths are
relative to ``score/datarouter`` of this repository.

Interfaces of the Component
---------------------------

The interfaces of the component architecture and the external interfaces are
realized by the following source code interfaces. The interface documentation is
part of the headers.

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Architecture interface
     - Source code interface
     - Realizing units
   * - :need:`logic_arc_int__logging__shm`
     - ``ISharedMemoryReader`` (``score_logging`` ``score/mw/log``), used by
       ``SourceSession``
     - Data router core
   * - :need:`logic_arc_int__logging__session_ctrl_channel`
     - ``MessagePassingServer`` with ``MessagePassingServer::ISession`` and
       ``ISessionHandle``, messages in ``data_router_messages.h``
     - Message passing server, data router core
   * - DLT over UDP multicast (:need:`comp_req__data_router__dlt_server`)
     - ``UdpStreamOutput``, ``DltLogChannel``
     - DLT server and channels
   * - Configuration client (Unix domain socket)
     - ``UnixDomainServer::ISession``, ``IDiagnosticJobHandler``
     - Unix domain server, configuration
   * - Command line (``-c``, ``-C``)
     - ``Options``
     - Configuration

.. toctree::
   :titlesonly:
   :maxdepth: 1

   logging_architecture
   shm_apis
