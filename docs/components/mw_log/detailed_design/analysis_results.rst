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



Analysis Results
################

Evidence for the :doc:`implementation inspection checklist <chklst_impl_inspection>`
of the :doc:`detailed design <index>`. It is kept apart from the design, because
the status changes more often than the design.

The expected structural coverage is 100 % line and 100 % branch. The coverage
for reporting is measured on QNX, the unit tests are also executed there.
The numbers below are from the host run.

The analyses follow the S-CORE C++ guidelines:
`C++ <https://eclipse-score.github.io/score/main/contribute/development/cpp/index.html>`_,
`coding guidelines (MISRA C++:2023)
<https://eclipse-score.github.io/score/main/contribute/development/cpp/coding_guidelines.html>`_,
`code analysis
<https://eclipse-score.github.io/score/main/contribute/development/cpp/code_analysis.html>`_
and for the Rust bridge crates
`Rust <https://eclipse-score.github.io/score/main/contribute/development/rust/index.html>`_.
Findings are documented as open points in the remarks of the checklist and each
one is handled by a follow-up ticket.

Results
=======

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Analysis
     - Result
   * - Unit tests (``//score/mw/log/...``)
     - 17 of 19 passed. The 2 slog tests are QNX only and skipped on the host.
   * - clang-tidy (backend units)
     - No findings. Slog targets are skipped on the host.
   * - ASan, UBSan, LSan
     - No findings.
   * - TSan
     - No findings. 17 of 19 tests passed, the 2 slog tests are skipped on the host.
   * - Memcheck (valgrind)
     - No errors and no definite leaks in the unit tests of ``detail/data_router``
       (incl. ``shared_memory``), ``detail/file_recorder``,
       ``detail/wait_free_producer_queue``, ``detail/syslog`` and the registrants in
       ``backend`` (9 targets, valgrind 3.22.0, ``--leak-check=full``,
       ``--errors-for-leak-kinds=definite``, ``--track-origins=yes``). The slog
       test is QNX only and skipped. Googletest prints the padding bytes of mock
       return values (``optional<SlotHandle>``, ``expected<int, Error>``) and
       valgrind reports them as uninitialised values.
   * - Compiler warnings
     - Warnings are errors in the build. The build passes with gcc and with the
       clang toolchain used for clang-tidy.
   * - Coverity
     - Not done yet.
   * - Line coverage ``detail/data_router`` (incl. ``shared_memory``)
     - 98.3 % (1219 lines, 21 missed), branches 96.3 % (187, 7 missed)
   * - Line coverage ``detail/file_recorder``
     - 98.6 % (284 lines, 4 missed), branches 95 % (20, 1 missed)
   * - Line coverage ``detail/wait_free_producer_queue``
     - 93.1 % (319 lines, 22 missed), branches 93.1 % (58, 4 missed)
   * - Line coverage ``backend`` registrants
     - 100 %
   * - Line coverage ``detail/syslog``
     - 100 % (108 lines, 0 missed), branches 86.1 % (36, 5 missed).
       ``backend/syslog_registrant.cpp`` has no test and no coverage.
   * - Line coverage ``detail/slog``
     - Slog is QNX only. The tests are executed on QNX, the coverage numbers of that
       run are not yet attached, see the open points of IMPL_02_01.


.. _mw_log_backend_missing_coverage:

Missing Coverage
================

The target is 100 % line and branch coverage. The table lists everything that is
missing in the host run, with the cause. The coverage tool reports ``LCOV_EXCL``
lines as missed because no justification file is given. They count as covered only
if the exclusion is accepted in the review. All other entries are fixed in follow-up
tickets.

.. list-table::
   :header-rows: 1
   :widths: 25 12 10 28 25

   * - File
     - Lines
     - Branch lines
     - Cause
     - Action
   * - ``backend/syslog_registrant.cpp``
     - 29-32
     -
     - No test for the registrant.
     - Add a registrant test like ``file_registrant_test``.
   * - ``detail/data_router/data_router_backend.cpp``
     - 41, 42, 44, 45
     - 37, 123
     - Limit of the slot number to ``SlotIndex`` not tested. ``default`` case of the
       log level switch not tested.
     - Add tests: capacity above the limit, log level out of range. Or justify.
   * - ``detail/data_router/data_router_message_client_impl.cpp``
     - 117-120, 191-196
     - 116, 190, 351
     - The lines are in ``LCOV_EXCL`` regions (race and error handling that cannot
       be triggered). Line 351: ``sender_ == nullptr`` in ``SendMessage`` is not
       tested.
     - Accept the exclusions in the review. Add a test for line 351.
   * - ``detail/data_router/data_router_message_client_impl.h``
     - 41-43
     -
     - ``constexpr`` function, evaluated at compile time.
     - Justify or exclude.
   * - ``detail/data_router/shared_memory/shared_memory_reader.cpp``
     -
     - 38
     - Branch of ``while (true)`` not covered.
     - Check and justify or add a test.
   * - ``detail/data_router/shared_memory/shared_memory_writer.h``
     - 4 (not located)
     - 1 (not located)
     - Reported by the summary table, not in the line data. Probably ``constexpr``
       code.
     - Locate and justify.
   * - ``detail/file_recorder/dlt_message_builder.cpp``
     - 228-231
     - 228
     - ``default`` case in an ``LCOV_EXCL`` region (unreachable for valid enum).
     - Accept the exclusion in the review.
   * - ``detail/syslog/syslog_backend.cpp``
     -
     - 83, 123
     - ``default`` case of the level conversion. Failure branch of an assert.
     - Test with a log level out of range. Assert: death test or justify.
   * - ``detail/syslog/syslog_recorder_factory.cpp``
     -
     - 29, 31, 40
     - Failure branches of asserts (memory resource, backend, syslog instance).
     - Death tests or justify.
   * - ``detail/wait_free_producer_queue/alternating_reader.cpp``
     -
     - 51
     - One outcome of a combined condition is not covered.
     - Add a test.
   * - ``detail/wait_free_producer_queue/linear_control_block.h``
     - 46-49
     -
     - ``constexpr`` function, evaluated at compile time.
     - Justify or exclude.
   * - ``detail/wait_free_producer_queue/wait_free_alternating_writer.cpp``
     - 68, 79-102
     - 61, 93
     - ``LCOV_EXCL`` regions (concurrent switch of the block), braces of the region.
     - Accept the exclusions in the review.
   * - ``detail/slog``, ``backend/slog_registrant.cpp``
     - open
     - open
     - QNX only. The result of the QNX run is not attached.
     - Attach the QNX coverage and list the gaps here.

.. _mw_log_backend_requirement_tests:

Requirements with incomplete test evidence
==========================================

Three requirements of the component had no test linked to them. Two were tested
already, the tests are now linked with ``RecordProperty("PartiallyVerifies", "<id>")``.
The table lists the linked tests and what is still not tested.

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Requirement
     - Linked tests
     - Still missing
   * - ``comp_req__log__send_to_datarouter``
     - - ``DataRouterBackendFixture.LogLevelVerbose``: a message is logged through
         the backend and read back from the shared memory.
       - ``DatarouterMessageClientFixture.ConnectToDatarouterShouldSendConnectMessage``:
         the client sends the connect message to the Datarouter.
       - ``DatarouterMessageClientFixture.AcquireNotifyShouldSendExpectedAcquireResponse``:
         the client answers an acquire request with the expected response.
     - No test checks that a message reaches the real Datarouter process. Only
       ``test_logging_showcase.py`` in ``reference_integration`` does this, and it
       is not linked to the requirement.
   * - ``comp_req__log__inactive_logstream``
     - - ``DataRouterRecorderFixtureWithLogLevelCheck.WillObtainEmptySlotForInsufficentLogLevel``:
         for a level that is not enabled no slot is taken, so nothing is written.
       - ``DataRouterRecorderFixtureWithLogLevelCheck.DisablesOrEnablesLogAccordingToLevel``:
         logging is enabled or disabled according to the configured level.
     - No test on the level of ``LogStream``, the stream the user writes to. This
       belongs to ``score_baselibs``.
   * - ``comp_req__log__avoid_signal_processing``
     - None.
     - The requirement says that no signal handler is registered. No test shows
       this. It is only checked by reading the code: there is no ``sigaction`` or
       ``signal()`` call, SIGTERM is only blocked with ``pthread_sigmask`` in
       ``SignalHandling`` and ``DatarouterMessageClientImpl::BlockTermSignal``.
       ``signal_handling_test`` tests only the blocking helper.

For the other requirements of the component architecture at least one source or
build file references the requirement id. This only shows the link, not that the
requirement is fully verified.
