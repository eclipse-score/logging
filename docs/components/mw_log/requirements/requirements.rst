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

Component Requirements
######################

.. document:: mw::log Backend Component Requirements
   :id: doc__mw_log_backend_requirements
   :version: 1
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__requirements_comp

mw::log
-------

.. comp_req:: Avoid Signal Processing
   :id: comp_req__log__avoid_signal_processing
   :version: 1
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__error_handling_isolation, feat_req__logging__asil_support
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall not register any signal handler.

.. comp_req:: File Descriptor Flags
   :id: comp_req__log__file_descriptor_flags
   :version: 1
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__security_log_access
   :status: valid
   :satisfied_by: comp__mw_log_backend

   The component shall set the ``FD_CLOEXEC`` (or ``O_CLOEXEC``) flag on all the file
   descriptors it owns.

   Note: This requirement can be satisfied in unit-test, by checking
   ``int flags = fcntl(fd, F_GETFD); assert((flags != -1) && (flags & FD_CLOEXEC));`` for every
   file descriptor owned by the component.

.. comp_req:: DLT Verbose Mode
   :id: comp_req__log__dlt_verbose_mode
   :version: 1
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__compat_dlt
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall implement DLT verbose mode. Generation of messages shall be done in
   DLT-conform format, allowing messages to be sent by Datarouter without additional
   reprocessing.

.. comp_req:: AUTOSAR Log and Trace Specification
   :id: comp_req__log__autosar_log_trace_spec
   :version: 1
   :reqtype: Functional
   :security: YES
   :safety: QM
   :derived_from: feat_req__logging__compat_dlt, feat_req__logging__severity_levels
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall implement "Specification of Log and Trace for Adaptive Platform". Release of
   specification used shall be in agreement with the rest of Adaptive AUTOSAR stack.

.. comp_req:: Shared Memory File Permissions
   :id: comp_req__log__shm_file_permissions
   :version: 1
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__security_log_access, feat_req__logging__config_permissions
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall ensure that the shared-memory files it creates are not writable
   by group and others.

   .. note::
      mw::log creates a shared memory file for each app.

System Backend
--------------

*System logger backend shall forward the logs to the native system logger.*

.. comp_req:: Forward to System Logger
   :id: comp_req__log__forward_to_system_logger
   :version: 2
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__log_sinks_console
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall forward the logs to the native system logger mechanism when the system
   logger backend is used.

   .. note::
      mw::log shall use slogger2 on QNX and syslog(3) on Linux.
   Note: slogger2 shall be used for QNX. syslog(3) shall be used for Linux.

.. comp_req:: System Backend Activation
   :id: comp_req__log__system_backend_activation
   :version: 1
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__logging__config_on_demand
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall enable the system logger backend if and only if the log mode contains
   "kSystem".

Non-Functional Requirements
---------------------------

.. comp_req:: Local Allocation Strategy
   :id: comp_req__log__local_allocation_strategy
   :version: 1
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__resource_runtime, feat_req__logging__asil_support
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall use local allocators to avoid using global heap. Global heap allocation (if
   any) shall be limited to initialization phase of application lifecycle.

.. comp_req:: No Endless Loops
   :id: comp_req__log__no_endless_loops
   :version: 1
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support, feat_req__logging__resource_performance
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall not contain unbound loops or loops with unchecked exit conditions.

.. comp_req:: Avoid Locks
   :id: comp_req__log__avoid_locks
   :version: 1
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support, feat_req__logging__resource_performance
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall acquire locks only with a bounded configurable wait time.
   If the wait time is exceeded, mw::log shall abort the acquisition and drop
   the affected log message instead of continuing to wait.

.. comp_req:: Priority Inversion Protection
   :id: comp_req__log__priority_inversion_protection
   :version: 1
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support, feat_req__logging__resource_performance
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall protect all mutual exclusion mechanisms against priority
   inversion by using priority inheritance or priority ceiling.

.. comp_req:: Cross-Locking Prevention
   :id: comp_req__log__cross_locking
   :version: 1
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support, feat_req__logging__error_handling_isolation
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall not block the execution of any thread, in the same or another
   application, because another thread is logging repeatedly.

.. comp_req:: Index and Size Checking
   :id: comp_req__log__index_size_checking
   :version: 1
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall check indices in data structures and sizes of data accessed or copied for
   plausibility, to avoid high runtime utilization through long iterations or data corruption.
   If a check fails, mw::log shall discard the affected log message without performing the access.

.. comp_req:: Memory Bound Checking
   :id: comp_req__log__memory_bound_checking
   :version: 1
   :reqtype: Non-Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__logging__asil_support
   :status: valid
   :satisfied_by: comp__mw_log_backend

   mw::log shall check memory boundaries before writing to shared memory.
   If a check fails, mw::log shall discard the affected log message without performing the write.
