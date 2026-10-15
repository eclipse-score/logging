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

```{document} mw::log Syslog Backend Detailed Design
:id: doc__mw_log_syslog_backend_design
:status: draft
:version: 1
:safety: ASIL_B
:security: YES
:realizes: wp__sw_implementation
```

`SyslogBackend` implements `mw::log::detail::Backend` for Linux.
`SyslogRecorderFactory` creates it. The factory is registered for
`LogMode::kSystem` in `backend/syslog_registrant.cpp`.
`TextRecorder` uses it in the same way as `SlogBackend` on QNX.

`SyslogBackend` stores `LogRecord` objects in a `CircularAllocator`.
`ReserveSlot()` acquires a slot and `FlushSlot()` sends its contents to syslog
and releases the slot. If no slot is available, `ReserveSlot()` returns no value.
No log data leaves the process before `FlushSlot()` runs.

`FlushSlot()` maps `LogLevel` to a syslog(3) priority.
It sends the application ID, context ID (each limited to four characters), and
payload in one formatted call.
`kOff` and out-of-range levels are ignored, the slot is released nevertheless.

`Syslog::openlog()` runs once during construction.
The `Syslog` object seam enables host tests without a running syslog daemon.

```{uml} _assets/mw_log_recorders.puml
```

The sequence below shows a single log call from `LogStream` construction
through to the `syslog(3)` call made when the stream is destroyed and the slot
is flushed:

```{uml} _assets/syslog_backend_sequence.puml
```
