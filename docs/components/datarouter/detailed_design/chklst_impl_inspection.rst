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

.. document:: Datarouter Implementation Inspection Checklist
  :id: doc__datarouter_impl_inspection
  :status: draft
  :version: 1
  :safety: QM
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

Scope: this checklist covers the Datarouter daemon (:need:`comp__datarouter`, ``score/datarouter``).
The parts of :doc:`logging_architecture` about ``mw::log`` are context and were not compared with the code.

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
     - Detailed design structure and UML views were reviewed against the template.
       The diagrams pass the ``architectural_design`` target
       ``//score/datarouter/doc/design:datarouter_architectural_design``.
       Sources remain in ``score/datarouter/doc/design`` and are included through
       symlinks.
     -
   * - IMPL_01_02
     - Is the implementation according to specification?
     - Check if the linked component requirements are fulfilled
       and detailed design also matches architecture description.
     - no
     - The detailed design matches the component architecture (units, interfaces,
       see :doc:`index`). Component requirements without a test link were linked
       to existing tests.

       Following issues need to be addressed:

       - Nothing tests that messages are dropped when a source exceeds its quota
         (:need:`comp_req__data_router__dlt_quota_bw_reduction`).
         ``DataRouter::SourceSession`` has no unit test.
       - Close remaining requirement-to-test traceability gaps: `#319 <https://github.com/eclipse-score/logging/issues/319>`_
     - Requirement tests: `#319 <https://github.com/eclipse-score/logging/issues/319>`_.
       Other points: To be added.
   * - IMPL_01_03
     - Are the design decisions and constraints documented?
     - Check also for plausibility of these.
     - no
     - The decisions and constraints in :doc:`index` were inspected against the
       code and refer to diagrams.

       Following issues need to be addressed:

       - Constraints that are only visible in the code and in no diagram. Show
         them in the diagrams:

         - Watchdog values (1000 ms deadline, 3 misses): `watchdog defaults in AcquireWatchdogConfig`_
         - Tick of the worker thread (100 ms): `kTimeoutInMs in RunWorkerThread`_
         - Statistics period (10 s): `kStatisticsLogPeriodUs in socketserver.cpp`_
         - Order of the filters: `FilterAndCall in dlt_log_server.h`_
         - Build flags: `flags in build_configuration_flags/BUILD`_
         - Path of ``log-channels.json``: `kLogChannelsPath in socketserver.cpp`_
         - Handling of ``ENOBUFS``: `ENOBUFS check in NotifyAcquireRequest`_
         - Input from clients is untrusted: `unsupported message types in message_passing_server.cpp`_
       - No diagram shows the bandwidth quota per source: `CheckAndSetQuotaEnforcement in data_router.cpp`_
       - :doc:`shm_apis` describes a POSIX mutex and a Datarouter that modifies
         the shared memory. The code uses wait-free alternating buffers and a
         read-only mapping. Correct the text.
     - To be added
   * - IMPL_01_04
     - Are all external libraries used by the component specified in the detailed design?
     - Check the automated dependency analysis.
       Also make sure ASIL rated units also only use ASIL rated libraries.
     - no
     - Dependency and FFI metadata are maintained in Bazel. The generated
       :doc:`dependencies` page lists the dependencies of each unit.
       The Datarouter is QM, the rule "ASIL units only use ASIL-rated
       libraries" does not apply to it.

       Dependency/configuration findings remain:

       - Correct the unresolved ``//score/mw/log/detail/dlt_trace``
         dependency (``mw::log`` backend, see the ``mw::log`` checklist).
       - Replace the deprecated ``//score/mw/log:log`` target (removal in
         v0.8, used by 9 non-test targets) with
         ``@score_baselibs//score/mw/log``. Example:
         `datarouter_lib dependencies in BUILD`_
       - Migrate uses of the deprecated ``high_resolution_steady_clock``
         target to the supported replacement after compatibility review. Used
         in `HighResolutionSteadyClock in data_router.cpp`_
       - Remove the declared but unused dependencies ``string_literal``
         (`string_literal dependency in error/BUILD`_) and
         ``static_reflection_with_serialization/visitor``
         (`visitor dependency in BUILD`_), if confirmed unused for all
         affected configurations.
       - Mark the mock libraries (``*_mock``) ``testonly``, for example the
         `mock targets in BUILD`_.
       - Remove ``visibility`` entries of packages that do not exist here
         (``//platform/aas/...``, ``//pqr/...``): `visibility in BUILD`_,
         `visibility in dlt_filetransfer_trigger_lib/BUILD`_ and
         `visibility in test/ut/etc/BUILD`_.
       - Get the safety ratings of ``score_communication`` ``message_passing``,
         ``rapidjson``, ``score_baselibs`` ``result`` and ``string_manipulation``
         confirmed.
     - To be added
   * - IMPL_02_01
     - Are the static and dynamic code analysis reports verified for violations?
     - All violations in ASIL related code must be justified. This includes the checks of coding guidelines.
     - no
     - Refer to the generated :doc:`verification report </verification_report/module_verification_report>`
       and revision-specific CI results. A host run (Linux x86_64) of the unit
       tests, clang-tidy, ASan/UBSan/LSan, TSan and Memcheck shows no findings.
       Line and branch coverage are 82.6 % and 76.1 %. The QNX tests were not run.

       Following issues need to be addressed:

       - Resolve or justify coverage gaps (target 100 %), including the missing
         unit tests of ``DataRouter`` and ``SourceSession`` see: `#318 <https://github.com/eclipse-score/logging/issues/318>`_
       - Integrate and verify the required CodeQL analysis see: `#336 <https://github.com/eclipse-score/logging/issues/336>`_.
       - 9 of 159 suppressions have no justification and 94 use AUTOSAR C++14
         ids instead of MISRA C++:2023 (list in #318).
       - Memcheck is not part of CI.
     -
       - Coverage: `#318 <https://github.com/eclipse-score/logging/issues/318>`_.
       - CodeQL `#336 <https://github.com/eclipse-score/logging/issues/336>`_.
       - Other points: To be added.
   * - IMPL_02_02
     - Do manual checks, that are derived from the coding guideline, find no safety critical error?
     - Check this for the programming language used (e.g. C++ <link_to_checks_list>, Rust <link_to_checks_list>)
     - no
     - The manual MISRA C++:2023 checks are not done yet. The Datarouter has no
       Rust code.

       Following issues found in `C++ guideline <https://eclipse-score.github.io/score/main/contribute/development/cpp/coding_guidelines.html>`_:

       - The coding guideline asks to avoid the prefix ``I`` for interfaces
         (``IPersistentDictionary``, ``ISession``, ``ILogParser``,
         ``ILogParserFactory``, ``IDiagnosticJobHandler``, ``IDltLogServer``).
         Decide whether this is an accepted coding-guideline deviation.
     - To be added
   * - IMPL_03_01
     - Do the UID of the interface in component documentation match the implemented interface names of the unit?
     - Compare interface UIDs (which contains the interface name) in component architecture/detailed design documentation
       with public interfaces in source code (e.g. API headers, traits, public types/functions).
     - no
     - The detailed design maps each architecture interface to its source code
       interface (:doc:`index`).

       Following issues found:

       - The names in the code do not match the UIDs, the mapping exists only in
         the detailed design. Examples: :need:`logic_arc_int__logging__shm` is
         implemented by ``ISharedMemoryReader`` (``score/mw/log``),
         :need:`logic_arc_int__logging__session_ctrl_channel` by
         ``MessagePassingServer``, ``ISession`` and ``ISessionHandle``.
       - The DLT output (UDP multicast) and the configuration socket
         (``UnixDomainServer``) have no UID in the architecture.

       Action: decide whether to adapt the UIDs, rename the interfaces or accept
       the mapping as a documented deviation.
     - To be added
   * - IMPL_03_02
     - Are detailed design and source code consistent and is the respective traceability established ?
     - Check if available static and dynamic design diagrams and the textual descriptions match the code
       (e.g. naming of interfaces, units, functions/operations/messages, data types).
       Check if the folder/file names of the units and its source code matches the intended functionality.
       For example if a unit is named "communication" it should not contain code for "data processing".
     - no
     - The texts of :doc:`logging_architecture` and :doc:`shm_apis`, the README,
       the units table and the diagrams were compared with the code. The session,
       watchdog, early disconnect and configuration session diagrams match.

       Following issues found:

       - :doc:`logging_architecture` says that after the first quota violation all
         further records of the source are dropped and the flag is not reset. The
         code clears the flag with the statistics report every 10 s
         (``DataRouter::SourceSession``). Decide which is intended and change that
         one.
       - ``file_transfer_feature_class_diagram.puml`` shows handler factories
         that only tests use. In the code ``DltLogServer`` owns the handler
         (``fthandler_``).
       - ``package-datarouter.puml`` lists classes that do not exist
         (``AraPerPersistentDictionary``, ``SysedrHandler`` and others).
       - ``dlt_message_filtering_backend.puml`` is incomplete and names the wrong
         file for the defaults (see ``DltLogServer::FilterAndCall``).
       - The persistent logging diagrams (``persistent_logging_*``,
         ``persistent-cleanup-*``) describe classes that are not in this
         repository. Remove them or move them to the repository with the
         implementation.
       - ``ipc_status_quo.puml``, ``context-details.puml``,
         ``seq-tracing-startup.puml`` and ``remote_logging.plantuml`` are used in
         no document and were not compared with the code.
       - The parts about ``mw::log`` (``seq-trace.puml``,
         ``dlt_message_filtering_frontend.puml``) belong to the ``mw::log``
         frontend: `#327 <https://github.com/eclipse-score/logging/issues/327>`_
       - The README statistics example misses the fields ``rate``, ``quota rate``,
         ``quota enforcement`` and ``IPC count``.
     - ``mw::log`` context: `#327 <https://github.com/eclipse-score/logging/issues/327>`_.
       Other points: To be added.

.. _watchdog defaults in AcquireWatchdogConfig: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/include/daemon/message_passing_server.h#L66-L67
.. _kTimeoutInMs in RunWorkerThread: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/src/daemon/message_passing_server.cpp#L224
.. _kStatisticsLogPeriodUs in socketserver.cpp: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/src/daemon/socketserver.cpp#L56
.. _FilterAndCall in dlt_log_server.h: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/include/daemon/dlt_log_server.h#L210-L235
.. _flags in build_configuration_flags/BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/build_configuration_flags/BUILD
.. _kLogChannelsPath in socketserver.cpp: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/src/daemon/socketserver.cpp#L54
.. _ENOBUFS check in NotifyAcquireRequest: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/src/daemon/message_passing_server.cpp#L579-L585
.. _unsupported message types in message_passing_server.cpp: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/src/daemon/message_passing_server.cpp#L409-L412
.. _CheckAndSetQuotaEnforcement in data_router.cpp: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/datarouter/data_router.cpp#L413-L443
.. _datarouter_lib dependencies in BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/BUILD#L389
.. _HighResolutionSteadyClock in data_router.cpp: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/datarouter/data_router.cpp#L170
.. _string_literal dependency in error/BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/error/BUILD#L29
.. _visitor dependency in BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/BUILD#L309
.. _mock targets in BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/BUILD#L911-L983
.. _visibility in BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/BUILD#L75
.. _visibility in dlt_filetransfer_trigger_lib/BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/dlt_filetransfer_trigger_lib/BUILD#L30-L32
.. _visibility in test/ut/etc/BUILD: https://github.com/eclipse-score/logging/blob/359df78e36f6bf1a3a0830cb61525302b0bd3460/score/datarouter/test/ut/etc/BUILD#L26
