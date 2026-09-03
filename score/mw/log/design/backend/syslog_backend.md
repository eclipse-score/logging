# SyslogBackend

`SyslogBackend` implements `mw::log::detail::Backend` and is the Linux counterpart
of the QNX `SlogBackend`: it is selected by `SyslogRecorderFactory` when the
configured log mode contains `kSystem` and the target platform is Linux
(`target_compatible_with = ["@platforms//os:linux"]`), wrapping it in a
`TextRecorder` the same way `SlogBackend` is wrapped on QNX.

Like `SlogBackend`, it stores `LogRecord`s in a `CircularAllocator` between
`ReserveSlot()` and `FlushSlot()`. `ReserveSlot()` and `GetLogRecord()` only
hand out and look up slots in that buffer; no data leaves the process until
`FlushSlot()` is called.

`FlushSlot()` converts the `LogRecord`'s `LogLevel` to a syslog(3) priority via
`ConvertMwLogLevelToSyslogPriority()` and forwards the record's app ID, context
ID and payload to the injected `score::os::Syslog` seam as a single
`syslog(priority, "%.*s,%.*s: %.*s", ...)` call, which glibc delivers through
`vsyslog(3)`. `LogLevel::kOff` (and any out-of-range level) maps to
`SyslogPriority::kInvalid` and is dropped instead of being forwarded, mirroring
`SlogBackend`'s handling of its own invalid level.

The `score::os::Syslog` seam is opened once, in the constructor's `Init()`, via
`openlog(app_id, LOG_PID | LOG_NDELAY, LOG_USER)`; the object-seam wrapper
(`Syslog` interface / `SyslogImpl` / `MockSyslog`) is what makes `SyslogBackend`
host-unit-testable without a real syslog daemon.

<img alt="MW_LOG_RECORDERS" src="https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/eclipse-score/logging/refs/heads/main/score/mw/log/design/backend/mw_log_recorders.puml">

The sequence below shows a single log call from `LogStream` construction
through to the `syslog(3)` call made when the stream is destroyed and the slot
is flushed:

<img alt="SyslogBackendSequenceDesign" src="https://www.plantuml.com/plantuml/proxy?src=https://raw.githubusercontent.com/eclipse-score/logging/refs/heads/main/score/mw/log/design/backend/syslog_backend_sequence.puml">
