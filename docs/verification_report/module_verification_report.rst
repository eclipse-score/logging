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

Verification Report
===================


.. mod_ver_report:: Logging Module Verification Report
   :id: mod_vrep__logging__report
   :post_template: module_verification_report
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :verification_method: test_and_inspection
   :belongs_to: mod__logging
   :realizes: wp__verification_module_ver_report

Structural Coverage
-------------------

Unit test coverage of :need:`comp__datarouter` and :need:`comp__mw_log_backend`,
without the QNX-only ``slog`` backend.

.. score-coverage-report:: mod__logging
   :title: Logging

Deviations:

- Coverage below the targets of :need:`doc__verification_plan`.
- Tooling for merging component test coverage is not available yet
  (`coverage_tool#39 <https://github.com/eclipse-score/coverage_tool/issues/39>`_).
- No result per component
  (`coverage_tool#51 <https://github.com/eclipse-score/coverage_tool/issues/51>`_).

.. toctree::
   :maxdepth: 1

   stats
