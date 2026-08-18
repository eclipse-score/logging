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

Module
======

.. mod:: Logging
   :id: mod__logging_repo
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :includes: comp__datarouter, comp__mw_log_backend

   The logging module provides a standardized logging framework for C++ and Rust projects using Bazel build system. It includes components for log routing allowing for flexible log management and integration with various logging backends. The module is designed to be extensible and configurable to meet the needs of different applications and environments.

Module View
-----------

.. mod_view_sta:: Logging module view
   :id: mod_view_sta__logging__static_view
   :version: 1
   :includes: comp__datarouter, comp__mw_log_backend
   :belongs_to: mod__logging_repo

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_module(need(), needs) }}

Module Documents
----------------

.. toctree::
   :maxdepth: 1

   manuals/index
   release/release_note
   safety_mgt/index
