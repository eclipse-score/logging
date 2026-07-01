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

.. _component_detailed_design_datarouter:

Detailed Design
###############

.. document:: Data Router Detailed Design
   :id: doc__data_router_detailed_design
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__sw_implementation

Detailed Design for Component: Data Router
==========================================

Description
-----------

The Data Router is a daemon process responsible for receiving log entries from
client applications via IPC (message passing and Unix domain sockets) and
routing them to the appropriate output backends, primarily the DLT (Diagnostic
Log and Trace) subsystem.

The component consists of the following internal units:

- ``applications`` — entry point and command-line option parsing
- ``daemon`` — core server logic: DLT log channel, message passing server, socket server, UDP output
- ``dlt`` — non-verbose DLT output implementation
- ``file_transfer`` — DLT file transfer support
- ``logparser`` — log entry deserialization and parsing
- ``persistency`` — persistent storage abstraction for log configuration
- ``persistent_logging`` — system EDR (Event Data Recorder) integration
- ``unix_domain`` — Unix domain socket client
- ``configuration`` — dynamic configuration session management

Implementation Structure
------------------------

.. code-block:: text

    score/datarouter/src/
    ├── applications/
    │   ├── datarouter_app.cpp/.h     # DatarouterApp: top-level application class
    │   ├── main_nonadaptive.cpp      # main() entry point
    │   └── options.cpp/.h            # Command-line option parsing
    ├── configuration/
    │   └── dynamic_config/           # Dynamic config session interface + stub
    ├── daemon/
    │   ├── dlt_log_channel.cpp       # Bridges incoming log entries to DLT
    │   ├── dlt_log_server.cpp        # Accepts connections, dispatches entries
    │   ├── diagnostic_job_handler.cpp # Handles diagnostic control jobs
    │   ├── diagnostic_job_parser.cpp  # Parses diagnostic job payloads
    │   ├── log_entry_deserialization_visitor.cpp  # Visitor for deserializing entries
    │   ├── message_passing_server.cpp # score_baselibs message passing IPC server
    │   ├── persistentlogging_config.cpp # Persistent logging configuration
    │   ├── priority_boost.cpp        # Thread priority management
    │   ├── socketserver.cpp/.h       # Unix domain socket server
    │   ├── socketserver_config.cpp   # Socket server configuration
    │   ├── udp_stream_output.cpp     # UDP multicast DLT output
    │   ├── utility.cpp               # Shared utility functions
    │   └── verbose_dlt.cpp           # Verbose DLT mode output
    ├── dlt/
    │   ├── nonverbose_dlt_impl/      # Non-verbose DLT output implementation
    │   └── nonverbose_dlt_stub/      # Stub for build configurations without DLT
    ├── file_transfer/
    │   ├── file_transfer_handler_factory.hpp  # Factory for file transfer stream handlers
    │   └── file_transfer_stub/       # Stub for build configurations without file transfer
    ├── logparser/
    │   └── logparser.cpp             # Deserializes raw log frames into log entries
    ├── persistency/
    │   ├── i_persistent_dictionary.h         # Interface for persistent key-value store
    │   ├── persistent_dictionary_factory.hpp # Factory for creating the active dictionary impl
    │   └── stub_persistent_dictionary/       # No-op stub implementation
    ├── persistent_logging/
    │   ├── isysedr_handler.h                 # Interface for system EDR handler
    │   └── persistent_logging_stub/          # Stub EDR handler for non-EDR builds
    └── unix_domain/
        └── unix_domain_client.cpp            # Unix domain socket client

Rationale Behind Decomposition into Units
------------------------------------------

The daemon is split into independent units to allow compile-time feature
selection via Bazel build flags (``dlt_output_enable``, ``file_transfer``,
``persistent_logging``, ``enable_dynamic_configuration``). Each optional
feature has a corresponding stub target so that the daemon builds correctly
regardless of which features are enabled, without dead-code ifdefs in the
core logic.

The ``daemon/`` unit owns the IPC surface (socket server + message passing
server) and dispatches to whichever output backends are compiled in. This
keeps the routing logic centralized while the backends remain independently
replaceable.

Design Decisions
----------------

There are no formally recorded design decisions for this component.
The :need:`dec_rec__log__explicit_init` decision record covers planned
initialization changes that affect the startup sequence of this daemon.

Design Constraints
------------------

- The datarouter daemon must not depend on ``mw::log`` for its own internal
  logging to avoid circular initialization.
- All IPC between client applications and the daemon uses ``score_baselibs``
  message passing infrastructure to avoid direct socket dependencies in
  client libraries.
- Optional features (file transfer, persistent logging, dynamic config) are
  controlled exclusively via Bazel build flags; no runtime feature detection
  is performed.
