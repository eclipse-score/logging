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
========================

This section is reserved for middleware-specific documentation.

.. comp:: Logging Component
   :id: comp__mw_logging
   :security: YES
   :safety: ASIL_B
   :status: valid
   :implements: logic_arc_int__logging__logging
   :belongs_to: feat__logging

   This is the logging component library responsible for selecting the appropriate log sinks based on configuration at runtime. It can perform tasks such as log formatting, filtering, and composite backend selection based on runtime context and configuration. The logging component is designed to be extensible, allowing for custom logging backend to be added as needed.
