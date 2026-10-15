<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->
# SlogBackend

```{document} mw::log Slog Backend Detailed Design
:id: doc__mw_log_slog_backend_design
:status: draft
:version: 1
:safety: ASIL_B
:security: YES
:realizes: wp__sw_implementation
```

`SlogBackend` implements `mw::log::detail::Backend` for QNX. `SlogRecorderFactory`
creates it. The factory is registered for `LogMode::kSystem` in
`backend/slog_registrant.cpp`, see [backend registration](backend_registration.md).
`TextRecorder` uses it in the same way as `SyslogBackend` on Linux, with
`kCheckLogLevelForConsole` set to false.

`SlogBackend` stores `LogRecord` objects in a `CircularAllocator`. The number of
slots is limited to the range of `SlotIndex`. `ReserveSlot()` acquires a slot and
returns no value if all slots are in use. `FlushSlot()` sends the contents of the
slot to the slog2 buffer and releases the slot. No log data leaves the process
before `FlushSlot()` runs.

`FlushSlot()` maps `LogLevel` to a slog2 verbosity:

| `LogLevel` | slog2 verbosity |
| --- | --- |
| `kVerbose` | `SLOG2_DEBUG2` |
| `kDebug` | `SLOG2_DEBUG1` |
| `kInfo` | `SLOG2_INFO` |
| `kWarn` | `SLOG2_WARNING` |
| `kError` | `SLOG2_ERROR` |
| `kFatal` | `SLOG2_CRITICAL` |
| `kOff` and out-of-range levels | `SLOG2_INVALID_VERBOSITY` |

The application ID, the context ID (each limited to four characters) and the
payload are sent in one formatted `slog2f()` call. The result of the call is
ignored. In contrast to the syslog backend, the invalid verbosity is passed on to
`slog2f()`. The slog2 code of the log entry is used on QNX.

The constructor registers a slog2 buffer set once. The set and the buffer are named
after the application ID. There is one buffer of 16 pages of 4 kB (64 kB) and the
default verbosity is `SLOG2_DEBUG2`. If the registration fails, the error is
reported by `ReportInitializationError()` and logging continues.

The `Slog2` object is injected through the constructor, so that the backend can be
tested without the QNX slog2 service.

The backend is part of the class diagram of the recorders:

```{uml} _assets/mw_log_recorders.puml
```
