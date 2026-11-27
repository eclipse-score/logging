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

Architecture Coverage
---------------------

Scope
~~~~~

The architecture coverage covers the component architecture elements (static
views ``comp_arc_sta`` and dynamic views ``comp_arc_dyn``) of all components of
:need:`mod__logging`:

- :need:`comp__datarouter` and :need:`comp__mw_log_backend`, owned by this
  repository.
- :need:`comp__log` (Log Frontend), owned by ``score_baselibs`` and imported
  into this report.

An architecture element is covered when at least one test links it with
``FullyVerifies`` or ``PartiallyVerifies``. The tests are the unit tests under
``//score/...``. Their results come from the coverage job of the CI, which does
not run the component tests (``integration`` tag), so links from component
tests are not part of this report.

.. needtable::
   :filter: type in ["comp_arc_sta", "comp_arc_dyn"] and ("comp__datarouter" in belongs_to or "comp__mw_log_backend" in belongs_to or "comp__log" in belongs_to or "comp__log[version==1]" in belongs_to)
   :columns: id;title;safety;status;testlink
   :colwidths: 25,20,8,8,39
   :style: table
   :sort: id

Verdict
~~~~~~~

- **Datarouter, mw::log Backend:** every architecture element is covered by at
  least one test. The tests verify the elements partially, together they cover
  the flows shown in the views.
- **Log Frontend:** the static view (:need:`comp_arc_sta__log__static_view`) is
  covered by tests of this repository. The three remaining elements
  (:need:`comp_arc_sta__log__class_diagram`, :need:`comp_arc_dyn__log__log_op`,
  :need:`comp_arc_dyn__log__log_to_level`) describe the Rust frontend
  (``score_log`` crate) of ``score_baselibs``. They are verified in
  ``score_baselibs`` and are not linked from this repository: the Rust tests run
  here report one result per test target and cannot carry verification links.
- **Log Frontend requirements:** the component requirements of :need:`comp__log`
  are shown above without test links for the same reason. Their verification is
  reported in ``score_baselibs``.
- The architecture elements have the status ``valid``; the architecture
  inspection is not part of this report.

Elements without a verifying test are listed in :ref:`log_statistics`.

.. toctree::
   :maxdepth: 1

   stats
