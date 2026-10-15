<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->
# Backend Registration

```{document} mw::log Backend Registration Detailed Design
:id: doc__mw_log_backend_registration_design
:status: draft
:version: 1
:safety: ASIL_B
:security: YES
:realizes: wp__sw_implementation
```

The backends (file, remote, syslog, slog, custom) register themselves for a
`LogMode` during static initialization. The frontend does not depend on the
backends. A backend is part of an application when the application depends on its
library in `backend/BUILD`.

## Registration

The table `gBackendCreators` in `backend_table.h` (`score_baselibs`) has one entry
per `LogMode`: `kConsole` 0, `kFile` 1, `kRemote` 2, `kSystem` 3, `kCustom` 4 and
one spare entry. An entry holds a function pointer (`RecorderCreatorFn`) or
`nullptr`, which means that no backend is registered.

Each file `backend/<name>_registrant.cpp` contains:

- a creator function (for example `CreateSyslogRecorder()`), which creates the
  recorder factory of the backend and returns the recorder, and
- a namespace-scope object of `BackendRegistrant` (for example `kSyslogRegistrant`).
  Its constructor calls `RegisterBackend()`, which writes the function pointer into
  the table before `main()`.

| File | `LogMode` | Recorder factory | Availability |
| --- | --- | --- | --- |
| `file_registrant.cpp` | `kFile` | `FileRecorderFactory` | build flag `config_KFile_Logging` |
| `remote_registrant.cpp` | `kRemote` | `RemoteDltRecorderFactory` | build flag `Remote_Logging` |
| `syslog_registrant.cpp` | `kSystem` | `SyslogRecorderFactory` | Linux only |
| `slog_registrant.cpp` | `kSystem` | `SlogRecorderFactory` | QNX only |
| `custom_registrant.cpp` | `kCustom` | `CustomRecorderFactory` | build flag `Custom_Logging` |

`kSystem` is used by two backends. Only one of them exists on a platform, so there
is no conflict. The library targets are built with `alwayslink = True`. Without it
the linker would drop the registrant object, because nothing refers to it. The
registrant sources are only part of a target when the build flag is set.

The table is constant-initialized and the registrant object only writes into it.
The deviations from the static analysis rules for the unused registrant variable
(A3-3-2, M0-1-3, M0-1-9) are justified in the registrant sources.

## Use at runtime

`RegistryAwareRecorderFactory` (`score_baselibs`) reads the configuration and
creates the recorders:

- For each configured `LogMode` with a registered backend, the recorder is created
  by `CreateRecorderForMode()`.
- If a configured mode has no registered backend, an initialization error is
  reported and the console recorder is used instead (once).
- If no recorder was created, or the configuration cannot be read, only the console
  recorder is used. If this is not registered either, an `EmptyRecorder` is used.
- One recorder is returned as it is. Several recorders are combined in a
  `CompositeRecorder`.
- `CreateRecorderFromLogMode()` returns an `EmptyRecorder` and reports an error if
  the mode is not registered.

```{uml} _assets/backend_registration.puml
```
