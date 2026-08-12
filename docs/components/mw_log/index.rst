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

   The ``mw::log`` implementation spans two repositories:

    ``score_baselibs/score/mw/log`` provides the frontend API, console backend, and static recorder composition.

    ``score_logging/score/mw/log`` provides concrete file, remote/DLT, and slog recorders together with their backend
    integration artifacts.

   Both repositories own their own safety plan, phase gates, and governance. The
   cross-repository contract is the Recorder interface and static ``backend_table``
   registration. The existing C ABI seam is not used for static composition.
   Runtime plugin loading remains experimental and is not a supported
   production capability.

   .. uml:: mw_log_repository_boundary.puml

.. toctree::
   :titlesonly:
   :maxdepth: 1

   requirements
   architecture/index
   detailed_design/index
   design_decisions/explicit_init
