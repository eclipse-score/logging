<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->
# SyslogBackend

`SyslogBackend` implements `mw::log::detail::Backend` for Linux.
`SyslogRecorderFactory` selects it when the log mode contains `kSystem`.
`TextRecorder` uses it in the same way as `SlogBackend` on QNX.

`SyslogBackend` stores `LogRecord` objects in a `CircularAllocator`.
`ReserveSlot()` acquires a slot and `FlushSlot()` sends its contents to syslog.
No log data leaves the process before `FlushSlot()` runs.

`FlushSlot()` maps `LogLevel` to a syslog(3) priority.
It sends the application ID, context ID, and payload in one formatted call.
`kOff` and out-of-range levels are ignored.

`Syslog::openlog()` runs once during construction.
The `Syslog` object seam enables host tests without a running syslog daemon.

<img alt="MW_LOG_RECORDERS" src="https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/eclipse-score/logging/refs/heads/main/score/mw/log/design/backend/mw_log_recorders.puml">

The sequence diagram shows a log call from stream construction to syslog(3).

<img alt="SyslogBackendSequenceDesign" src="https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/eclipse-score/logging/refs/heads/main/score/mw/log/design/backend/syslog_backend_sequence.puml">

<img alt="MW_LOG_RECORDERS" src="https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/eclipse-score/logging/refs/heads/main/score/mw/log/design/backend/mw_log_recorders.puml">

The sequence below shows a single log call from `LogStream` construction
through to the `syslog(3)` call made when the stream is destroyed and the slot
is flushed:

<img alt="SyslogBackendSequenceDesign" src="https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/eclipse-score/logging/refs/heads/main/score/mw/log/design/backend/syslog_backend_sequence.puml">
