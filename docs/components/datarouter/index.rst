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


Datarouter Component
====================

.. comp:: Datarouter
   :id: comp__datarouter
   :version: 1
   :security: YES
   :safety: QM
   :status: valid
   :belongs_to: feat__logging

   Datarouter is the DLT (Diagnostic log and trace) daemon executable. It reads records from source shared-memory ring buffers, manages source
   sessions, routes messages to configured channels via UDP multicast, and reports source statistics and message drops.

.. toctree::
   :titlesonly:
   :maxdepth: 1

   requirements/index
   detailed_design/logging_architecture
   detailed_design/shm_apis
