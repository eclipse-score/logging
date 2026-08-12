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


Datarouter
=========================

.. comp:: Datarouter
   :id: comp__data_router
   :security: YES
   :safety: ASIL_B
   :status: valid
   :implements: logic_arc_int__logging__logging
   :belongs_to: feat__logging

   Datarouter is the DLT daemon executable. It reads records from source shared-memory ring buffers, manages source
   sessions, routes messages to configured channels via UDP multicast, and reports source statistics and message drops.

.. toctree::
   :titlesonly:
   :maxdepth: 1
   :glob:

   *
