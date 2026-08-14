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

Logging Feature Architecture
=============================

The logging feature has two parts: the ``mw::log`` library that applications
link against and ship as part of their own binary, and the ``datarouter``
daemon that runs as its own process. Applications never talk to ``datarouter``
directly; ``mw::log``'s remote backend writes log records into a shared-memory
segment and hands control over to ``datarouter`` through a shared-memory and
`mw::com message-passing <https://eclipse-score.github.io/communication/latest/message_passing.html>`_
session; guaranteeing Freedom from interference (FFI) for the application and its
logging subsystem. Rust applications reach that same remote backend too, via
the ``score_log_bridge`` described below, so ``datarouter`` sees one uniform
set of sessions regardless of which frontend produced them.

mw::log
-------

The ``mw::log`` library spans across two repositories:

``score_baselibs/score/mw/log`` provides the Cpp frontend API, console backend, static recorder composition and
the ``score_log`` Rust facade that gives Rust code the same API surface, modelled on Rust's own ``log`` crate.
This allows `baselibs <https://eclipse-score.github.io/baselibs/main/>`_ and `mw::com <https://eclipse-score.github.io/communication>`_
to log to the same backends as applications, and providing a common logging API for all
components. The library is linked into applications and shipped as part of their binary.

``score_logging/score/mw/log`` provides concrete file, remote/DLT, and slog
recorders, plus the ``score_log_bridge`` that implements score_log
`Log` trait and forwards records over FFI into those same
recorders, so a Rust application ends up writing through the identical
remote backend.

Both repositories own their own safety plan, phase gates, and governance. The
cross-repository contract is the Recorder interface and static ``backend_table``
registration. The existing C ABI seam is not used for static composition.
Runtime plugin loading remains experimental and is not a supported
production capability.

.. uml:: _assets/mw_log_repository_boundary.puml

Datarouter
----------

The Datarouter is the Diagnostic log and trace (DLT) daemon.
See :doc:`the Datarouter component </components/datarouter/index>`
for its requirements and detailed design.

The diagram highlights the logging components and traces the
remote DLT path a log message takes once it leaves an application, including
a Rust application whose ``score_log_bridge`` registers as a recorder the same
way the C++ remote backend does and writes into its own shared-memory segment
for ``datarouter`` to read.

.. uml:: _assets/remote_logging.puml
