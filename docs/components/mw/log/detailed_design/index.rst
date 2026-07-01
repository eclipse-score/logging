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

.. _component_detailed_design_mw_log:

Detailed Design
###############

.. document:: Logging Component Detailed Design
   :id: doc__mw_logging_detailed_design
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_implementation

Detailed Design for Component: mw::log
=======================================

Description
-----------

The ``mw::log`` component is the client-side logging library consumed by
application code. It selects and registers the appropriate log sinks
(backends) based on runtime configuration and routes log records through
a wait-free producer queue to the chosen backend(s).

The component consists of the following internal units:

- ``backend`` — log sink registration (remote/DLT, file, QNX slog2, custom)
- ``detail/common`` — shared utilities: DLT framing, statistics reporting, log entry deserialization
- ``detail/data_router`` — IPC client that forwards log entries to the Data Router daemon
- ``detail/file_recorder`` — writes log entries directly to DLT binary files
- ``detail/slog`` — QNX slog2 output backend
- ``detail/utils`` — SIGTERM blocking utilities for safe thread creation during backend startup
- ``detail/wait_free_producer_queue`` — lock-free multi-producer single-consumer queue
- ``legacy_non_verbose_api`` — backwards-compatible non-verbose tracing API
- ``rust`` — Rust log bridge: FFI adapter and explicit initialization support

Implementation Structure
------------------------

.. code-block:: text

    score/mw/log/
    ├── backend/
    │   ├── custom_registrant.cpp     # Registers a user-provided custom recorder
    │   ├── file_registrant.cpp       # Registers the DLT file recorder backend
    │   ├── remote_registrant.cpp     # Registers the Data Router (remote DLT) backend
    │   └── slog_registrant.cpp       # Registers the QNX slog2 backend
    ├── detail/
    │   ├── common/
    │   │   ├── dlt_format.h                  # DLT frame layout constants
    │   │   ├── helper_functions.h            # Serialization helpers
    │   │   ├── istatistics_reporter.h        # Interface for statistics reporting
    │   │   ├── log_entry_deserialize.h       # Log entry deserialization utilities
    │   │   └── statistics_reporter.h         # Default statistics reporter
    │   ├── data_router/
    │   │   ├── data_router_backend.h         # Backend that wraps the message client
    │   │   ├── data_router_message_client.h  # Interface for IPC to the daemon
    │   │   ├── data_router_message_client_impl.h   # Concrete message client
    │   │   ├── data_router_recorder.h        # Log recorder using the message client
    │   │   ├── message_passing_config.h      # IPC channel configuration
    │   │   ├── message_passing_factory.h     # Factory interface for IPC channels
    │   │   └── remote_dlt_recorder_factory.h # Factory for the remote recorder
    │   ├── file_recorder/
    │   │   ├── dlt_message_builder.h         # Constructs DLT binary frames
    │   │   ├── dlt_message_builder_types.h   # Type definitions for DLT frames
    │   │   ├── file_recorder.h               # Writes DLT frames to a file
    │   │   ├── file_recorder_factory.h       # Factory for file recorder instances
    │   │   └── svp_time.h                    # Timestamp utilities
    │   ├── slog/
    │   │   ├── slog_backend.h                # QNX slog2 backend implementation
    │   │   └── slog_recorder_factory.h       # Factory for slog2 recorder instances
    │   ├── utils/
    │   │   └── signal_handling/
    │   │       ├── signal_handling.h         # SIGTERM block/unblock helpers for thread creation
    │   │       └── signal_handling.cpp       # Implementation
    │   └── wait_free_producer_queue/
    │       └── alternating_control_block.h   # Lock-free queue control block
    ├── legacy_non_verbose_api/
    │   └── tracing.cpp/.h                    # Non-verbose tracing compatibility shim
    └── rust/
        ├── score_log_bridge/src/adapter.cpp  # C++ FFI adapter for Rust log crate
        └── score_log_bridge_cpp_init/        # Explicit initialization for Rust+C++ environments
            ├── score_log_bridge_init.cpp/.h  # Init API consumed by applications
            └── examples/main.cpp             # Usage example

Rationale Behind Decomposition into Units
------------------------------------------

**Backend abstraction**: Each log sink is an independent registrant that can
be selected at runtime based on the JSON configuration file. This allows the
same library to target console-only, file-only, or remote (DLT) output
without recompilation.

**Wait-free producer queue**: Log calls from application threads must never
block. The ``wait_free_producer_queue`` decouples log record production
from consumption by the backend, enabling lock-free multi-producer behavior.

**Data Router client isolation**: The ``detail/data_router/`` unit encapsulates
all IPC details for sending log entries to the daemon. Changing the IPC
transport (e.g. from Unix sockets to shared memory) only affects this unit.

**Rust bridge as a separate unit**: The Rust bridge is kept separate so that
pure C++ builds do not incur any Rust toolchain dependency. The
``score_log_bridge_cpp_init`` unit provides explicit initialization for
mixed Rust/C++ applications (see :need:`dec_rec__log__explicit_init`).

Design Decisions
----------------

- :need:`dec_rec__log__explicit_init` — Explicit initialization of logging:
  describes the planned unification of Rust and C++ logging initialization
  into a single explicit ``score_log_bridge_cpp_init`` call, replacing the
  current lazy JSON-based initialization for C++.

Design Constraints
------------------

- Log calls from application code must be wait-free on the hot path to avoid
  introducing latency in safety-relevant contexts.
- The component must not link against the Data Router daemon directly; all
  communication is through the ``score_baselibs`` message passing IPC so that
  applications remain independent of the daemon binary.
- The file recorder backend writes raw DLT binary format to avoid a runtime
  dependency on the ``dlt-daemon`` system service when local file logging
  is sufficient.
- QNX slog2 support is conditionally compiled; the backend is excluded on
  non-QNX targets to keep Linux builds free of QNX-specific headers.
