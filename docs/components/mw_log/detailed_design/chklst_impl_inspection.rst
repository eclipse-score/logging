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

.. document:: mw::log Backend Implementation Inspection Checklist
  :id: doc__mw_log_backend_impl_inspection
  :status: draft
  :version: 1
  :safety: ASIL_B
  :security: YES
  :realizes: wp__sw_implementation_inspection

Implementation Inspection Checklist
===================================

Purpose
-------

The purpose of this checklist is to collect the topics to be checked during implementation,
i.e. in the detailed design and the source code of the units.

The checklist shall be agnostic to which programming language is used. Differences shall be treated
by linking to C++ or Rust specific documentation.

Conduct
-------

As described in the concept :need:`doc_concept__wp_inspections` the following "inspection roles" are expected to be filled:

- content responsible (author): <contributor/committer explicitly named here, who is the main author, as can be seen in config mgt tooling>
- reviewer: <contributor/committer explicitly named here, who is the main content reviewer, must be different from content responsible>
- moderator: <committer explicitly named here, who is is the safety manager, security manager or quality manager initiating the inspection>

Checklist
---------

It is mandatory to fill in the "passed" column with "yes" or "no" for each checklist item and additionally to add in the remarks why it is passed or not passed.
In case of "no" an issue link to the issue tracking system has to be added in the last column (if not solved in the same issue).
See also :need:`doc_concept__wp_inspections` for further information about reviews in general and inspection in particular.

.. list-table:: Implementation Checklist
   :header-rows: 1
   :widths: 10,25,30,6,35,20

   * - Review ID
     - Acceptance Criteria
     - Guidance
     - Passed
     - Remarks
     - Issue link
   * - IMPL_01_01
     - Is the design according to guidelines?
     - see :need:`gd_temp__detailed_design` and :need:`doc_concept__imp_concept`
       (e.g. are the views done with the proposed UML diagrams)
     - yes
     -  Detailed design structure and available UML views were reviewed.
        Sources remain in their original paths and plant uml diagrams are included through symlinks.
        The slog backend and the syslog backend have no design document, their
        implementation is self-explanatory and serves as the design. The design
        of the backend registration is in ``score_baselibs`` (``score/mw/log/design``).
     -
   * - IMPL_01_02
     - Is the implementation according to specification?
     - Check if the linked component requirements are fulfilled
       and detailed design also matches architecture description.
     - yes
     - comp_req__log__avoid_signal_processing is coverd by clang-tidy checker.
     -
   * - IMPL_01_03
     - Are the design decisions and constraints documented?
     - Check also for plausibility of these.
     - yes
     - Backend design decisions and constraints were inspected against the
       implementation, including bounded work and message-size limits.
     -
   * - IMPL_01_04
     - Are all external libraries used by the component specified in the detailed design?
     - Check the automated dependency analysis.
       Also make sure ASIL rated units also only use ASIL rated libraries.
     - yes
     - Dependency and FFI metadata are maintained in Bazel. The direct
       dependencies outside of ``//score/mw/log`` are listed with::

         bazel query 'let l = kind(".*_library", //score/mw/log/...) except attr(testonly, 1, //score/mw/log/...) in labels(deps, $l) except //score/mw/log/...' --keep_going --output=label_kind

       Those without the ``FFI`` tag are listed with::

         bazel query 'let l = kind(".*_library", //score/mw/log/...) except attr(testonly, 1, //score/mw/log/...) in (labels(deps, $l) except //score/mw/log/...) except attr(tags, FFI, labels(deps, $l))' --keep_going --output=label

       Dependency/configuration findings:

       - ``//score/mw/log/detail/dlt_trace`` is not defined in this repository,
         so the ``shm_dma_enabled`` configuration does not build here.
         Accepted as deviation.
       - The ``high_resolution_steady_clock`` target is deprecated and its
         replacement is not available in the open source dependencies.
         Accepted as deviation.
     -
   * - IMPL_02_01
     - Are the static and dynamic code analysis reports verified for violations?
     - All violations in ASIL related code must be justified. This includes the checks of coding guidelines.
     - no
     - Static code analysis is covered by: AddressSanitizer + UndefinedBehaviorSanitizer + LeakSanitizer + ThreadSanitizer.

       Static code analysis with MISRA cpp23 checks is planned.

       Dynamic code analysis using valgrind is planned.
     - `#336 <https://github.com/eclipse-score/logging/issues/336>`_

       `#389 <https://github.com/eclipse-score/logging/issues/389>`_
   * - IMPL_02_02
     - Do manual checks, that are derived from the coding guideline, find no safety critical error?
     - Check this for the programming language used (e.g. C++ <link_to_checks_list>, Rust <link_to_checks_list>)
     - no
     - The manual MISRA C++:2023 checks are not done yet. Accepted as deviation,
       new tooling will be set up soon.

       Following issues found in `C++ guideline <https://eclipse-score.github.io/score/main/contribute/development/cpp/coding_guidelines.html>`_:

       - The coding guideline asks to avoid the prefix ``I`` for interfaces and to
         append ``Impl`` to implementations. The ``ISharedMemoryReader`` / ``IMessageBuilder``
         naming is accepted as coding-guideline deviation.
     -
   * - IMPL_03_01
     - Do the UID of the interface in component documentation match the implemented interface names of the unit?
     - Compare interface UIDs (which contains the interface name) in component architecture/detailed design documentation
       with public interfaces in source code (e.g. API headers, traits, public types/functions).
     - yes
     -
     -
   * - IMPL_03_02
     - Are detailed design and source code consistent and is the respective traceability established ?
     - Check if available static and dynamic design diagrams and the textual descriptions match the code
       (e.g. naming of interfaces, units, functions/operations/messages, data types).
       Check if the folder/file names of the units and its source code matches the intended functionality.
       For example if a unit is named "communication" it should not contain code for "data processing".
     - yes
     - Backend designs were reconciled with their implementations.
       The slog backend and the syslog backend have no design document, their
       implementation is self-explanatory (see IMPL_01_01). The design of the
       backend registration is in ``score_baselibs`` (see IMPL_01_01).
       The design of the wait-free producer queue (``detail/wait_free_producer_queue/README.md``, ``design/*.puml``)
       is not part of this detailed design and was not compared with the code.
     -
