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

.. document:: Logging Backend Architecture
   :id: doc__logging_backend_architecture
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__component_arch

Overview
--------

Supports logging on several backends (file,slog,remote,custom)

Description
-----------

<General Description>

<Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.>

<Design Constraints>

Rationale Behind Architecture Decomposition
*******************************************

Mandatory: A motivation for the decomposition or reason for not further splitting it into internal components.

.. note:: Common decisions across components / cross cutting concepts is at the higher level.

Static Architecture
-------------------

The components are designed to cover the expectations from the feature architecture
(i.e. if already exists a definition it should be taken over and enriched).

A component can optional also consist of lower level components to further structure the architecture. The component and its static views can also optionally use interfaces provided by other components.

.. comp:: Logging Backend
   :id: comp__logging_backend
   :security: YES
   :safety: ASIL_B
   :status: valid
   :implements: logic_arc_int__log_cpp__logging
   :belongs_to: feat__logging

   This is the logging component library responsible for selecting the appropriate log sinks based on configuration at runtime. It can perform tasks such as log formatting, filtering, and composite backend selection based on runtime context and configuration. The logging component is designed to be extensible, allowing for custom logging backend to be added as needed.


.. comp_arc_sta:: Logging Backend (Static View)
   :id: comp_arc_sta__log__sv
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :belongs_to: comp__logging_backend
   :fulfils: comp_req__log__avoid_signal_processing, comp_req__log__file_descriptor_flags, comp_req__log__dlt_verbose_mode, comp_req__log__autosar_log_trace_spec, comp_req__log__send_to_datarouter, comp_req__log__inactive_logstream, comp_req__log__shm_file_permissions, comp_req__log__forward_to_system_logger, comp_req__log__system_backend_activation, comp_req__log__local_allocation_strategy, comp_req__log__no_endless_loops, comp_req__log__avoid_locks, comp_req__log__cross_locking, comp_req__log__index_size_checking, comp_req__log__memory_bound_checking

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

not needed

Interfaces
----------

not needed

Decision Records
----------------

:need:`dec_rec__logging__explicit_init`
