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

mw::log Component Requirements
###############################

.. document:: Logging Component Requirements
   :id: doc__logging_requirements
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__requirements_comp

Log and Trace
-------------

*Log and Trace Framework consists of logging libraries and datarouter. The applications use
logging libraries to write logs. datarouter implements logging daemon functionality, and can
forward log messages from applications as DLT messages.*

.. comp_req:: Use Log and Trace Framework
   :id: comp_req__log__use_log_trace_framework
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__log_sources_user_app, feat_req__logging__log_sources
   :status: valid
   :belongs_to: comp__logging

   All output from logging and instrumentation based logging shall go through "mw::log"
   framework.


mw::log
-------

.. comp_req:: Avoid Signal Processing
   :id: comp_req__log__avoid_signal_processing
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__error_handling_isolation, feat_req__logging__asil_support
   :status: valid
   :belongs_to: comp__logging

   The component shall not register any signal handler.

.. comp_req:: File Descriptor Flags
   :id: comp_req__log__file_descriptor_flags
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__security_log_access
   :status: valid
   :belongs_to: comp__logging

   The component shall set the ``FD_CLOEXEC`` (or ``O_CLOEXEC``) flag on all the file
   descriptors it owns.

   Note: This requirement can be satisfied in unit-test, by checking
   ``int flags = fcntl(fd, F_GETFD); assert((flags != -1) && (flags & FD_CLOEXEC));`` for every
   file descriptor owned by the component.

.. comp_req:: DLT Verbose Mode
   :id: comp_req__log__dlt_verbose_mode
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__compat_dlt
   :status: valid
   :belongs_to: comp__logging

   mw::log shall implement DLT verbose mode. Generation of messages shall be done in
   DLT-conform format, allowing messages to be sent by Datarouter without additional
   reprocessing.

.. comp_req:: AUTOSAR Log and Trace Specification
   :id: comp_req__log__autosar_log_trace_spec
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__logging__compat_dlt, feat_req__logging__severity_levels
   :status: valid
   :belongs_to: comp__logging

   mw::log shall implement "Specification of Log and Trace for Adaptive Platform". Release of
   specification used shall be in agreement with the rest of Adaptive AUTOSAR stack.

.. comp_req:: Send Messages to Datarouter
   :id: comp_req__log__send_to_datarouter
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__log_sources_user_app, feat_req__logging__log_sinks_network
   :status: valid
   :belongs_to: comp__logging

   mw::log shall use logging library to send verbose messages to datarouter. Applications shall
   use mw::log interface to log verbose mode messages. Internally mw::log shall be using logging
   library.

.. comp_req:: Inactive LogStream Behavior
   :id: comp_req__log__inactive_logstream
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__filtering_log_levels, feat_req__logging__resource_performance
   :status: valid
   :belongs_to: comp__logging

   mw::log library shall not perform any useful activity if log level of LogStream object is
   insufficient for given context. LogStream object shall not generate serialized messages if
   the severity of the message does not allow it to be sent.

.. comp_req:: Shared Memory File Permissions
   :id: comp_req__log__shm_file_permissions
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__security_log_access, feat_req__logging__config_permissions
   :status: valid
   :belongs_to: comp__logging

   Logging shared-memory files shall have read-only posix file permission for group and others.
   The shared-memory file created by mw::log shall be read-only for group and others.

   Note: mw::log creates a shared memory file for each app.

System Backend
--------------

*System logger backend shall forward the logs to the native system logger.*

.. comp_req:: Forward to System Logger
   :id: comp_req__log__forward_to_system_logger
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__log_sinks_console
   :status: valid
   :belongs_to: comp__logging

   The system logger backend shall forward the logs to the native system logger mechanism.

   Note: Under QNX, slogger2 shall be used.

.. comp_req:: System Backend Activation
   :id: comp_req__log__system_backend_activation
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__config_on_demand
   :status: valid
   :belongs_to: comp__logging

   The system logger backend shall be enabled if and only if the log mode contains "kSystem".

Non-Functional Requirements
---------------------------

.. comp_req:: Local Allocation Strategy
   :id: comp_req__log__local_allocation_strategy
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__resource_runtime, feat_req__logging__asil_support
   :status: valid
   :belongs_to: comp__logging

   mw::log shall use local allocators to avoid using global heap. Global heap allocation (if
   any) shall be limited to initialization phase of application lifecycle.

.. comp_req:: No Endless Loops
   :id: comp_req__log__no_endless_loops
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support, feat_req__logging__resource_performance
   :status: valid
   :belongs_to: comp__logging

   mw::log shall not contain unbound loops or loops with unchecked exit conditions.

.. comp_req:: Avoid Locks
   :id: comp_req__log__avoid_locks
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support, feat_req__logging__resource_performance
   :status: valid
   :belongs_to: comp__logging

   mw::log shall be free of time-unbound locks and shall implement strategies to limit wait
   time. Time limit shall be defined based on requirements from functions on individual ECU.
   Mutual exclusion mechanisms shall include priority inversion protection.

.. comp_req:: Cross-Locking Prevention
   :id: comp_req__log__cross_locking
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support, feat_req__logging__error_handling_isolation
   :status: valid
   :belongs_to: comp__logging

   Cross-application and cross-thread dependencies shall be avoided. A thread logging in a loop
   shall not block other thread's execution.

.. comp_req:: Index and Size Checking
   :id: comp_req__log__index_size_checking
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support
   :status: valid
   :belongs_to: comp__logging

   Indices in data structures and sizes of data accessed or copied shall be checked for
   plausibility to avoid high runtime utilization through long iterations or data corruption.

.. comp_req:: Memory Bound Checking
   :id: comp_req__log__memory_bound_checking
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support
   :status: valid
   :belongs_to: comp__logging

   Memory boundaries shall be explicitly checked when writing to shared memory.
