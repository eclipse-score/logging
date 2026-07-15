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


Logging Documentation
=====================

This section is reserved for middleware-specific documentation.

.. comp:: Logging Component
   :id: comp__mw_logging
   :security: YES
   :safety: ASIL_B
   :status: valid
   :implements: logic_arc_int__logging__logging
   :belongs_to: feat__logging

   This is the logging component library responsible for selecting the appropriate log sinks based on configuration at runtime. It can perform tasks such as log formatting, filtering, and composite backend selection based on runtime context and configuration. The logging component is designed to be extensible, allowing for custom logging backend to be added as needed.

.. toctree::
   :titlesonly:
   :maxdepth: 1
   :glob:

   *

.. document:: mw/log Detailed Design
   :id: doc__mw_logging_detailed_design
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_implementation


Static View
-----------

.. comp_arc_sta:: mw/log Static View
   :id: comp_arc_sta__mw_logging__static_view
   :security: YES
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__log__use_log_trace_framework
   :belongs_to: comp__mw_logging

   .. uml:: detailed_design/mw_log_recorders.puml

   .. uml:: detailed_design/mw_log_file_backend.puml

   .. uml:: detailed_design/mw_log_datarouter_recorder.puml

   .. uml:: detailed_design/write_factory_design.puml

   .. uml:: detailed_design/class_diagram.puml

   .. uml:: detailed_design/verbose_logging_static.puml

   .. uml:: detailed_design/backend_registration_component_diagram.puml

   .. uml:: detailed_design/configuration_static.puml

   .. uml:: detailed_design/configuration_use_cases.puml

   .. uml:: detailed_design/error_domain.puml

   .. uml:: detailed_design/frontend_dependency_graph.puml

   .. uml:: detailed_design/mw_log_default_recorders.puml

   .. uml:: detailed_design/non_verbose_logging_static.puml

   .. uml:: detailed_design/verbose_console_logging_static.puml


Dynamic View
------------

.. comp_arc_dyn:: mw/log Dynamic View
   :id: comp_arc_dyn__mw_logging__dynamic_view
   :security: YES
   :safety: ASIL_B
   :status: valid
   :fulfils: comp_req__log__use_log_trace_framework
   :belongs_to: comp__mw_logging

   .. uml:: detailed_design/shared_memory_writer_allocandwrite.puml

   .. uml:: detailed_design/verbose_logging_sequence.puml

   .. uml:: detailed_design/wait_free_linear_buffer.puml

   .. uml:: detailed_design/wait_free_alternating_buffers.puml

   .. uml:: detailed_design/backend_registration_sequence_diagram.puml

   .. uml:: detailed_design/configuration_sequence.puml

   .. uml:: detailed_design/dynamic_backend_selection.puml

   .. uml:: detailed_design/rarf_activity_diagram.puml

   .. uml:: detailed_design/slot_drainer_action_diagram_design.puml

   .. uml:: detailed_design/slot_drainer_sequence_design.puml

   .. uml:: detailed_design/verbose_console_logging_sequence.puml
