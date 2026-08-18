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

Feature
###############

Abstract
========

The logging feature provides a standardized logging framework for C++ and Rust projects using Bazel build system.

For the main feature description and requirements, see the belonging `Feature <https://eclipse-score.github.io/score/main/features/log_trace/index.html>`_ in the project repository.

Change Request (CR) Guidelines
==============================

New contributors to the logging feature should start with the
:doc:`feature architecture </features/logging/architecture/index>` to understand the split between the
``mw::log frontend`` (owned by ``score_baselibs``) and the ``mw::log backend`` recorders implemented in this repository,
and how ``datarouter`` fits into the remote/DLT path.

Adding or integrating a backend (file, slog, remote/DLT, or a custom recorder) means implementing the
Recorder interface and registering it in the static ``backend_table``; the existing C ABI seam is not used
for static composition, and runtime plugin loading is experimental and not supported for production use.
The :doc:`architecture/index` and :doc:`/components/mw_log/detailed_design/index` sections document this
contract in detail, while the :doc:`/components/mw_log/requirements/index` and the safety analyses below
capture the safety and security constraints that any backend implementation must respect.

.. toctree::
   :hidden:

   architecture/index.rst
   architecture/chklst_arc_inspection.rst
   architecture/DR-001-logging.rst
   safety_planning/index.rst
