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


Data Router Documentation
=========================

This section is reserved for data router-specific documentation.

.. comp:: Data Router
   :id: comp__data_router
   :security: YES
   :safety: ASIL_B
   :status: valid
   :implements: logic_arc_int__logging__logging
   :belongs_to: feat__logging

   This is the datarouter component responsible for routing log messages to remote Diagnostics Log and Trace (DLT) backend.

.. toctree::
   :titlesonly:
   :maxdepth: 1
   :glob:

   *

.. document:: Data Router Detailed Design
   :id: doc__data_router_detailed_design
   :status: valid
   :safety: QM
   :security: YES
   :realizes: wp__sw_implementation


Static View
-----------

.. uml:: detailed_design/datarouter_class_diagram.puml

.. uml:: detailed_design/inter_process_communication.puml

.. uml:: detailed_design/mw_log_shared_memory_reader.puml


Dynamic View
------------

.. uml:: detailed_design/datarouter_backend_datarouterbackend.puml

.. uml:: detailed_design/shared_memory_reader_read.puml

.. uml:: detailed_design/datarouter_message_client_impl_connecttodatarouter.puml
