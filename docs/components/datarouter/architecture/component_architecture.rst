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

.. _datarouter_component_architecture:

Datarouter Component Architecture
=================================

.. document:: Datarouter Architecture
   :id: doc__datarouter_architecture
   :status: draft
   :version: 1
   :safety: QM
   :security: YES
   :realizes: wp__component_arch

Overview
--------

The Datarouter (:need:`comp__datarouter`) is the DLT daemon. It reads the log
records of the ``mw::log`` clients from shared memory, filters them and sends
them as DLT messages over UDP multicast.

Description
-----------

A client (the remote/DLT backend of ``comp__mw_log_backend``) connects to the
Datarouter over the control channel
(:need:`logic_arc_int__logging__datarouter_msg_client`), and the Datarouter
creates a source session for it. The Datarouter periodically asks each client to
hand over its buffer by a notification, then reads the log records from the
shared-memory ring buffer of the client
(:need:`logic_arc_int__logging__shared_memory`). Each record is parsed, filtered
by log level per channel and sent over the configured DLT channel.

Design decisions:

- Freedom from interference: the Datarouter is a separate process and only reads
  the shared memory of the clients.
- A client that does not answer must not stall the other clients: the acquire
  request is a non-blocking notification, and a watchdog removes a session after
  a limited number of missed deadlines.
- Messages are filtered early, with an own log level threshold per channel, to
  reduce the traffic.
- Deployment-specific configuration (``log-channels.json``) without a rebuild.

Design constraints:

- DLT output over UDP multicast only.
- ``log-channels.json`` is mandatory, ``class-id.json`` is optional.
- One worker thread serves all sessions.

The detailed design of the units is described in
:need:`doc__datarouter_detailed_design`.

Rationale Behind Architecture Decomposition
*******************************************

The component is not split into lower level components. It is a single daemon
process, decomposed into units by responsibility (session handling,
shared-memory reading, parsing, DLT output, configuration), so that each unit
can be tested and replaced independently. The units are described in the
detailed design.

.. note:: Common decisions across components / cross cutting concepts is at the higher level.

Static Architecture
-------------------

.. comp_arc_sta:: Datarouter (Static View)
   :id: comp_arc_sta__data_router__sv
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :belongs_to: comp__datarouter
   :fulfils: comp_req__data_router__dlt_server, comp_req__data_router__dlt_udp_multicast, comp_req__data_router__dlt_multiple_channels, comp_req__data_router__config_file, comp_req__data_router__dlt_channel_config, comp_req__data_router__dlt_message_filtering, comp_req__data_router__dlt_verbose_messages, comp_req__data_router__dlt_bw_quota_config, comp_req__data_router__dlt_quota_bw_reduction

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

.. comp_arc_dyn:: Initialization and Message Processing
   :id: comp_arc_dyn__data_router__message_processing
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :belongs_to: comp__datarouter
   :fulfils: comp_req__data_router__dlt_server, comp_req__data_router__dlt_verbose_messages, comp_req__data_router__dlt_multiple_channels

   The Datarouter creates the DLT server and a source session with its own
   parser for each new client. Each record read from shared memory is passed to
   the verbose, non-verbose or file transfer handler, filtered and sent over the
   DLT channel.

   .. uml:: ../detailed_design/_assets/datarouter_initialization_processing_sequence.puml

.. comp_arc_dyn:: Client Session Handling
   :id: comp_arc_dyn__data_router__client_session
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :belongs_to: comp__datarouter
   :fulfils: comp_req__data_router__dlt_verbose_messages

   The Datarouter requests the buffer of each client by a non-blocking
   notification, reads the acquired buffer and forwards the records. A client
   that does not answer does not stall the other clients; its session is
   removed by the watchdog after a limited number of missed deadlines.

   .. uml:: ../detailed_design/_assets/client_session_interaction_sequence.puml

.. comp_arc_dyn:: DLT Message Filtering
   :id: comp_arc_dyn__data_router__message_filtering
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :belongs_to: comp__datarouter
   :fulfils: comp_req__data_router__dlt_message_filtering, comp_req__data_router__config_file, comp_req__data_router__dlt_channel_config

   The log level threshold of a message is taken from ``log-channels.json`` (and
   ``class-id.json`` for non-verbose messages); a message above the threshold is
   dropped before it is copied to the UDP socket buffers.

   .. uml:: ../detailed_design/_assets/dlt_message_filtering_backend.puml

Interfaces
----------

The Datarouter uses the interfaces provided by ``comp__mw_log_backend``:
:need:`logic_arc_int__logging__shared_memory` and
:need:`logic_arc_int__logging__datarouter_msg_client`.
