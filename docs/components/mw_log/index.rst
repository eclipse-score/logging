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


mw::log
=====================

.. comp:: Logging Component
   :id: comp__mw_logging
   :security: YES
   :safety: ASIL_B
   :status: valid
   :implements: logic_arc_int__logging__logging
   :belongs_to: feat__logging

   This is the logging middleware library responsible for providing conrete backends for the supported recorder
   implementations. The logging component is designed to be extensible, allowing supported as well as custom logging
   backend to be added as needed.

   The ``mw::log`` implementation spans the ``score_baselibs`` (frontend) and ``score_logging`` (backend)
   repositories; see the :doc:`feature architecture </features/architecture/index>` for the repository boundary
   and cross-repository contract.

.. toctree::
   :titlesonly:
   :maxdepth: 1

   requirements/index
   detailed_design/index
   design_decisions/explicit_init
