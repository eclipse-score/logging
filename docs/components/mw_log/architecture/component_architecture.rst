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

.. _component_architecture:

Component Architecture
====================================

.. document:: mw::log Backend Architecture
   :id: doc__mw_log_backend_architecture
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__component_arch

Overview
--------

The ``mw::log`` Backend writes the log records created by the Log Frontend
(``score_baselibs``) to an output channel: the Datarouter (remote/DLT), the
system logger (syslog on Linux, slog on QNX), a local file or a custom backend.

Description
-----------

The backends are selected in two steps. At build time, the application links
the backends it needs, and each linked backend registers itself for a
``LogMode`` before ``main()``. At runtime, the logging configuration of the
application selects the ``LogMode`` values, and a recorder is created for each
of them that has a registered backend.

A recorder turns the log calls of a ``LogStream`` into a log record, and its
backend owns the slots that hold the records and writes a finished record to
the output channel. The remote/DLT backend hands the records over to
``comp__datarouter`` through a shared-memory ring buffer
(:need:`logic_arc_int__logging__shared_memory`) and uses a separate control
channel to connect to it (:need:`logic_arc_int__logging__datarouter_msg_client`).

Design decisions:

- Recorder and backend are separate abstractions, so a new output channel only
  needs a new backend.
- Logging does not block the application: slots are reserved without
  time-unbound locks, and a record that does not fit is dropped and counted
  instead of waited for.

Design constraints:

- Heap allocation only during initialization, local allocators afterwards.
- Syslog backend on Linux only, slog backend on QNX only.
- The file backend is not intended for production use (it uses a mutex to drain
  the slots).
- Input from the Datarouter is untrusted: sizes of shared-memory entries,
  messages and notifications are limited and checked.

The detailed design of the units is described in
:need:`doc__mw_log_backend_detailed_design`.

Rationale Behind Architecture Decomposition
*******************************************

The component is not split into lower level components. It is decomposed into
units by output channel (one recorder/backend pair per ``LogMode``), so that
each unit has a single responsibility and can be selected and tested
independently. The units are described in the detailed design.

.. note:: Common decisions across components / cross cutting concepts is at the higher level.

Static Architecture
-------------------

.. comp:: mw::log Backend
   :id: comp__mw_log_backend
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :implements: logic_arc_int__log_cpp__logging, logic_arc_int__logging__shared_memory, logic_arc_int__logging__datarouter_msg_client
   :belongs_to: feat__logging

   This is the logging component library responsible for selecting the appropriate log sinks based on configuration at runtime. It can perform tasks such as log formatting, filtering, and composite backend selection based on runtime context and configuration. The logging component is designed to be extensible, allowing for custom logging backend to be added as needed.


.. comp_arc_sta:: mw::log Backend (Static View)
   :id: comp_arc_sta__log__sv
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :belongs_to: comp__mw_log_backend
   :fulfils: comp_req__log__avoid_signal_processing, comp_req__log__file_descriptor_flags, comp_req__log__dlt_verbose_mode, comp_req__log__autosar_log_trace_spec, comp_req__log__shm_file_permissions, comp_req__log__forward_to_system_logger, comp_req__log__system_backend_activation, comp_req__log__write_to_local_file, comp_req__log__file_backend_activation, comp_req__log__local_allocation_strategy, comp_req__log__avoid_locks, comp_req__log__cross_locking, comp_req__log__index_size_checking, comp_req__log__memory_bound_checking

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

.. comp_arc_dyn:: Log Call via DataRouter Recorder
   :id: comp_arc_dyn__log__datarouter_log_call
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :belongs_to: comp__mw_log_backend
   :fulfils: comp_req__log__send_to_datarouter, comp_req__log__dlt_verbose_mode, comp_req__log__inactive_logstream, comp_req__log__log_loss_detect, comp_req__log__avoid_locks, comp_req__log__cross_locking, comp_req__log__memory_bound_checking

   One verbose log call in remote/DLT mode: the ``DataRouterRecorder`` reserves a
   slot, serializes the arguments in DLT verbose format and, when the
   ``LogStream`` is destroyed, flushes the record into the shared-memory ring
   buffer read by ``comp__datarouter``. A disabled log level, a missing slot or
   an argument that does not fit are handled without blocking and are counted.

   .. uml:: ../detailed_design/_assets/verbose_logging_sequence.puml

.. comp_arc_dyn:: Connection Setup to Datarouter
   :id: comp_arc_dyn__log__datarouter_connect
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :belongs_to: comp__mw_log_backend
   :fulfils: comp_req__log__send_to_datarouter, comp_req__log__avoid_signal_processing, comp_req__log__cross_locking

   The remote/DLT backend connects to ``comp__datarouter`` over the control
   channel in its own thread, so the application is not blocked while the
   Datarouter is not available. On a failure, the shared-memory file is unlinked
   and the client is stopped.

   .. uml:: ../detailed_design/_assets/datarouter_message_client_impl_connecttodatarouter.puml

.. comp_arc_dyn:: Log Call via System Backend
   :id: comp_arc_dyn__log__system_log_call
   :security: NO
   :safety: QM
   :status: valid
   :version: 1
   :belongs_to: comp__mw_log_backend
   :fulfils: comp_req__log__forward_to_system_logger

   One log call in system mode on Linux: the ``TextRecorder`` formats the
   record into a slot of the ``SyslogBackend``, which forwards it to the system
   logger (``syslog(3)``) when the slot is flushed.

   .. uml:: ../detailed_design/_assets/syslog_backend_sequence.puml

Interfaces
----------

.. logic_arc_int:: Log Record Shared-Memory
   :id: logic_arc_int__logging__shared_memory
   :included_by: feat__logging
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :fulfils: feat_req__logging__asil_support

   Shared-memory ring buffer carrying only serialized log records: the remote/DLT recorder
   backend writes into it, and ``comp__datarouter`` reads from it to route records onward to the
   network stack. Connection setup and buffer-acquire requests are not carried on this channel;
   see :need:`logic_arc_int__logging__datarouter_msg_client` for that.

.. logic_arc_int:: Control Channel
   :id: logic_arc_int__logging__datarouter_msg_client
   :included_by: feat__logging
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :fulfils: feat_req__logging__asil_support

   ``mw::com`` message-passing session between the remote/DLT recorder and its source session
   inside ``comp__datarouter``: used to initiate the connection and for ``comp__datarouter`` to
   send buffer-acquire requests back to the recorder. Carries no log record payload; see
   :need:`logic_arc_int__logging__shared_memory` for that.

Decision Records
----------------

:need:`dec_rec__logging__explicit_init`
