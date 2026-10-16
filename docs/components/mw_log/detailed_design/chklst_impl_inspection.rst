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
   :widths: 10,25,30,6,35,8

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
     - - Checked: the detailed design follows the template (description, decisions,
         constraints, units, static and dynamic views).
       - Checked: the views are UML diagrams (``uml`` directives, PlantUML sources).
       - Checked: the folder structure follows the module template.
       - Fixed: the design documents and PlantUML sources were moved from
         ``score/mw/log/design/backend`` into this folder (no symbolic links, one
         place to maintain).
       - Fixed: design texts for the slog backend and the backend registration were
         added.
       - Checked: the PlantUML diagrams are validated by the
         ``architectural_design`` target.
     -
   * - IMPL_01_02
     - Is the implementation according to specification?
     - Check if the linked component requirements are fulfilled
       and detailed design also matches architecture description.
     - no
     - The detailed design matches the component architecture (units, interfaces).
       Two requirements without a test link were linked to existing tests.

       Open Points:

       - The former datarouter design described a rate limit (fixed sleep after
         each incoming request), see
         `the removed text <https://github.com/eclipse-score/logging/blob/b2e3423a4a42aa79723e2042ab3511cec7523993/score/mw/log/design/backend/datarouter_backend/index.md#L232-L234>`_.
         The code has no such sleep. The text was replaced by the implemented
         limits (see ``datarouter_backend/index``) in commit ``57acd423``
         ("docs(mw/log): align datarouter backend design with code").
         There is no requirement for a rate limit: no component, feature or
         safety requirement in the documentation mentions it. Action: decide
         whether to add a requirement and implement the rate limit, or to accept
         the bounded notification handling as the only mitigation, and check
         that the safety analysis does not rely on a rate limit.
       - Test evidence of three requirements was incomplete, see
         :ref:`the requirements table <mw_log_backend_requirement_tests>`.
         ``send_to_datarouter`` and ``inactive_logstream`` were tested but not
         linked, the tests are linked now. Still open: ``avoid_signal_processing``
         has no test (negative property, only checked by reading the code),
         ``send_to_datarouter`` has no test of the delivery to the real Datarouter
         in this repository, and ``inactive_logstream`` has no ``LogStream`` level
         test in this repository. Action: add a static check or test that no
         signal handler is registered. Link the ``reference_integration`` showcase
         test and the ``score_baselibs`` ``LogStream`` tests to the requirements,
         or add tests.
     - To be added
   * - IMPL_01_03
     - Are the design decisions and constraints documented?
     - Check also for plausibility of these.
     - yes
     - Decisions and constraints are documented and were checked against the code
       (slot limit, payload limit, message sizes, bounded loops).
     -
   * - IMPL_01_04
     - Are all external libraries used by the component specified in the detailed design?
     - Check the automated dependency analysis.
       Also make sure ASIL rated units also only use ASIL rated libraries.
     - no
     - Dependencies are listed in the detailed design.

       Open Points:

       - ``//score/concurrency:thread_pool`` is a dependency in
         ``detail/data_router/BUILD`` (two targets) but no source file uses it.
         Action: remove the dependency.
       - The ``remote`` target in ``backend/BUILD`` refers to
         ``//score/mw/log/detail/dlt_trace`` in the ``shm_dma_enabled`` branch.
         This package does not exist in this repository, ``bazel query`` fails on
         it. Action: remove the branch or add the package.
       - ``//score/os/utils:high_resolution_steady_clock`` (``score_baselibs``) is
         deprecated. It is used by ``detail/data_router/shared_memory:common`` and
         ``detail/file_recorder:file_recorder``. Action: move to
         ``HighPrecisionLocalSteadyClock``.
       - The safety rating of ``score_communication`` ``score/message_passing``
         and ``score/memory/shared`` is not stated in the component
         documentation. The datarouter backend depends on both. Action: get the
         rating confirmed by the Communication team.
     - To be added
   * - IMPL_02_01
     - Are the static and dynamic code analysis reports verified for violations?
     - All violations in ASIL related code must be justified. This includes the checks of coding guidelines.
     - no
     - - Unit tests, clang-tidy, compiler warnings, ASan/UBSan/LSan, TSan and
         Memcheck: no findings.
       - Not done: Coverity.
       - The expected 100 % line and branch coverage is not reached, see
         :ref:`Missing Coverage <mw_log_backend_missing_coverage>`.

       Open Points:

       - Coverity is not done. Action: run it or provide the results, then
         update the analysis results.
       - ``backend:syslog`` was not part of the coverage scope
         (``quality/coverage/BUILD``) and the ``//score/mw/log`` facade does not
         depend on it. The scope was extended and ``detail/syslog`` is now
         measured. ``backend/syslog_registrant.cpp`` has no unit test, the other
         backends have one (``file_registrant_test``, ``remote_registrant_test``,
         ``slog_registrant_test``). Action: add a test for
         ``syslog_registrant.cpp`` and check that the syslog backend is part of
         the Linux build of the logging library.
       - ``data_router_backend.cpp`` lines 41 to 45 (limit of the number of slots
         to the range of ``SlotIndex``) are not covered by a test. Action: add a
         unit test or justify.
       - The slog backend is QNX only. Its tests are skipped on the host. The unit
         tests are executed on QNX, but the QNX coverage numbers are not
         attached. Coverage is tracked in eclipse-score/tooling#427. Action:
         attach the QNX test and coverage results and list the gaps.
       - 17 missed branches (data_router 7, file_recorder 1,
         wait_free_producer_queue 4, syslog 5). Action: fix or justify each entry
         of :ref:`Missing Coverage <mw_log_backend_missing_coverage>`.
       - A script check found 39 of 118 static-analysis suppressions
         (``coverity[]``, ``NOLINT``) in the backend units without a
         justification next to them. Most are POD structs
         (``autosar_cpp14_m11_0_1``) and may be covered by a block comment.
         Candidates outside the structs:
         ``data_router_message_client_impl.cpp:232``,
         ``data_router_message_client_factory_impl.cpp:72``,
         ``syslog_backend.cpp:124``, ``slog_backend.cpp:169`` and ``:172``,
         ``wait_free_linear_writer.cpp:165``. Action: review and add
         justifications where missing.
       - The suppressions use AUTOSAR C++14 rule ids
         (``coverity[autosar_cpp14_*]``). The project guideline is MISRA
         C++:2023, which replaces them. The code analysis page requires that
         findings which cannot be fixed are justified in the source. Action: map
         the suppressions to MISRA C++:2023 (rule mapping page) and check that
         each has a justification.
     - To be added
   * - IMPL_02_02
     - Do manual checks, that are derived from the coding guideline, find no safety critical error?
     - Check this for the programming language used (e.g. C++ <link_to_checks_list>, Rust <link_to_checks_list>)
     - no
     - The manual checks are not done yet. The checks list is the
       `C++ guideline <https://eclipse-score.github.io/score/main/contribute/development/cpp/coding_guidelines.html>`_
       (MISRA C++:2023) with the
       `code analysis <https://eclipse-score.github.io/score/main/contribute/development/cpp/code_analysis.html>`_
       page. The Rust bridge crates follow the
       `Rust guideline <https://eclipse-score.github.io/score/main/contribute/development/rust/index.html>`_.

       Open Points:

       - The manual checks of the coding guidelines (MISRA C++:2023 with the rule
         mapping of the S-CORE code analysis page) are not done. Action: run the
         checks and update this remark.
       - The coding guideline asks to avoid the prefix ``I`` for interfaces and to
         append ``Impl`` to implementations. ``ISharedMemoryReader`` in this
         repository and ``IMessageBuilder`` in ``score_baselibs`` use the prefix.
         Action: decide whether the names are an accepted deviation or are
         renamed.
     - To be added
   * - IMPL_03_01
     - Do the UID of the interface in component documentation match the implemented interface names of the unit?
     - Compare interface UIDs (which contains the interface name) in component architecture/detailed design documentation
       with public interfaces in source code (e.g. API headers, traits, public types/functions).
     - no
     - The mapping of the architecture interfaces to the source code interfaces is
       given in the detailed design.

       Open Points:

       - The names of the source code interfaces differ from the architecture
         interface UIDs, the mapping is only given in the detailed design.
         Examples:

         - ``logic_arc_int__log_cpp__logging`` is implemented by
           ``mw::log::Recorder``, which is defined in ``score_baselibs``
           (``score/mw/log/recorder.h``). The name has no relation to the UID.
         - ``logic_arc_int__logging__shm`` is realized by four names:
           ``SharedMemoryWriter``, ``ISharedMemoryReader``, ``SharedData`` and
           ``BufferEntryHeader``. No single name matches the UID.
         - ``logic_arc_int__logging__session_ctrl_channel`` is realized by
           ``DatarouterMessageClient`` and the messages in
           ``data_router_messages.h``. "Session control channel" does not
           appear in the code.
         - The internal interface ``mw::log::detail::Backend`` has no
           architecture UID.

         Action: decide whether to adapt the UIDs in the architecture, to
         rename the interfaces in the code, or to accept the mapping in the
         detailed design as a documented deviation. ``ISharedMemoryReader`` is
         also affected by the ``I`` prefix finding in IMPL_02_02.
     - To be added
   * - IMPL_03_02
     - Are detailed design and source code consistent and is the respective traceability established ?
     - Check if available static and dynamic design diagrams and the textual descriptions match the code
       (e.g. naming of interfaces, units, functions/operations/messages, data types).
       Check if the folder/file names of the units and its source code matches the intended functionality.
       For example if a unit is named "communication" it should not contain code for "data processing".
     - no
     - The file output, syslog and datarouter designs and diagrams were compared
       with the code and corrected. Checked and fixed: the two ``SlotDrainer``
       diagrams (loaded from ``score_baselibs`` by URL, now corrected copies in this
       folder), seven datarouter diagrams that were not used in any document, the
       missing slog and backend registration design, and the slot release in the
       syslog sequence diagram. Unit folders match their function.

       Open Points:

       - The design of the wait-free producer queue is in
         ``detail/wait_free_producer_queue/README.md`` and ``design/*.puml``. It
         is not part of this detailed design and was not compared with the code.
         The README example uses ``AlternatingReader``, which does not exist (the
         classes are ``AlternatingReadOnlyReader`` and
         ``AlternatingReaderProxy``). Its diagram links are relative to the
         repository root and do not resolve in the documentation. Action: correct
         the names, move the design into this folder like the other units, and
         compare the diagrams with the code.
     - To be added

Analysis Results
----------------

The results of the analyses, which are the evidence for the checklist items, are
documented on the following page:

.. toctree::

   analysis_results
