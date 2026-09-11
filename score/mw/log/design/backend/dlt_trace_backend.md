<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# DltTraceBackend (DLTv2 / GTL remote backend)

- [DltTraceBackend (DLTv2 / GTL remote backend)](#dlttracebackend-dltv2--gtl-remote-backend)
  - [Introduction](#introduction)
  - [Selection via the `shm_dma_enabled` flag](#selection-via-the-shm_dma_enabled-flag)
  - [Class overview](#class-overview)
  - [Slot lifecycle](#slot-lifecycle)
  - [Forwarding a record to the trace library](#forwarding-a-record-to-the-trace-library)
  - [Trace-library client creation](#trace-library-client-creation)
  - [Process teardown order issue of `score::memory::shared::MemoryResource`](#process-teardown-order-issue-of-bmwmemorysharedmemoryresource)
  - [Configuration](#configuration)
    - [Reuse of the DataRouter configuration (not the final solution)](#reuse-of-the-datarouter-configuration-not-the-final-solution)
  - [Related documents](#related-documents)

## Introduction

`DltTraceBackend` (in
[`detail/dlt_trace/dlt_trace_backend.h`](../../detail/dlt_trace/dlt_trace_backend.h))
is the second-generation remote DLT backend. Like the
[DataRouter backend](./datarouter_backend/README.md) it implements
`mw::log::Backend` and transports verbose/non-verbose log records off the
process for remote analysis. The key difference is the transport: instead of
writing into a DataRouter-owned ring buffer in shared memory, `DltTraceBackend`
forwards each record to the **Generic Trace Library (GTL)** through an owned
`score::analysis::tracing::ITraceLibrary` client. The GTL client is created by
`score::log_and_trace::dlt::CreateDltTraceLibrary()` (in
[`log_and_trace/plugins/dlt_plugin/code/trace_library/dlt_trace_factory.h`](../../../../log_and_trace/plugins/dlt_plugin/code/trace_library/dlt_trace_factory.h)).

Because the backend owns the trace-library instance, the backend's lifetime
bounds the trace-library lifetime and no process-wide singleton is created.

## Selection via the `shm_dma_enabled` flag

`DltTraceBackend` is registered under `LogMode::kRemote` by
[`dlt_trace_registrant.cpp`](../../detail/dlt_trace/dlt_trace_registrant.cpp),
i.e. it occupies the same backend slot as the classic DataRouter remote
backend. Which of the two remote backends is linked is decided at build time by
the `--//score/mw/log/flags:shm_dma_enabled` flag, resolved inside
[`backend/BUILD`](../../backend/BUILD):

| `shm_dma_enabled` | Remote backend linked | Transport |
|---|---|---|
| `False` (default) | DataRouter (`DataRouterRecorder`) — "DLTv1" | POSIX shared-memory ring buffer read by the DataRouter daemon |
| `True` | `DltTraceRecorder` (`DltTraceBackend`) — "DLTv2" | Generic Trace Library, shared memory with Direct Memory Access (DMA) capability |

Both variants expose themselves as `LogMode::kRemote`, so application code and
configuration select "remote logging" the same way regardless of which backend
is compiled in. See the
[`shm_dma_enabled` feature flag](../../README.md#feature-flags) in the top-level
`mw::log` README for the build command.

## Class overview

`DltTraceBackend` composes:

- a `CircularAllocator<LogRecord>` (`buffer_`) that pre-allocates the log-record
  slots — identical to the other backends, guaranteeing no runtime allocation on
  the hot path;
- an owned `std::unique_ptr<ITraceLibrary>` (`trace_library_`) — the GTL client;
- an `std::optional<TraceClientId>` (`client_id_`) — set only if
  `RegisterClient(BindingType::kDlt, app_description)` succeeded at construction.

Two constructors exist:

- **Production constructor** — creates and owns the GTL client via
  `CreateDltTraceLibrary(trace_shared_memory_size)`.
- **Injection constructor** — takes ownership of an externally supplied
  `ITraceLibrary`, used by unit tests to inject a mock.

`DltTraceRecorder` (in
[`detail/dlt_trace/dlt_trace_recorder.h`](../../detail/dlt_trace/dlt_trace_recorder.h))
pairs the backend with the DLT payload formatting (`DLTFormat` /
`DltArgumentCounter`) and applies the `appId` and per-context log-level
filtering from the `Configuration`.

## Slot lifecycle

`DltTraceBackend` follows the same `ReserveSlot()` / `GetLogRecord()` /
`FlushSlot()` contract as the other backends:

- `ReserveSlot()` acquires a free slot from the circular allocator and returns a
  `SlotHandle`; if the pool is exhausted it returns an empty optional (the
  message is dropped — logging is best-effort).
- `GetLogRecord()` returns the `LogRecord` bound to a handle so the recorder can
  write the formatted payload into it.
- `FlushSlot()` forwards the record (see below) and always returns the slot to
  the allocator so the pool cannot be exhausted even when forwarding is skipped.

## Forwarding a record to the trace library

On `FlushSlot()`:

1. If no client was registered (`client_id_` is empty), forwarding is skipped —
   there is no destination — but the slot is still released.
2. Otherwise the DLT metadata is populated from the log entry into a
   `score::analysis::tracing::DltProperties` (`app_id`, `ctx_id`, `log_level`,
   `number_of_arguments`) and wrapped in a `DltMetaInfo`.
3. The record payload is wrapped in a `LocalDataChunk` / `LocalDataChunkList`.
4. `trace_library_->Trace(client_id_, meta_info, chunk_list)` hands the record to
   the GTL; the result is intentionally ignored (best-effort delivery).

## Trace-library client creation

`CreateDltTraceLibrary()` builds a `GenericTraceAPIImpl` bound to:

- the DLT daemon channel `GetDltServerAddressName()`
  (`"dlt_trace_communication"`) served by the DLT trace plugin inside the LTPM
  daemon;
- the DLT trace-meta-data (TMD) shared-memory path `"/dev_dlt_tmd_"`;
- the DLT-specific metadata preparer (`DltLocalMetaDataPreparerFactory`).

It accepts an optional trace shared-memory size; when `std::nullopt` the library
default (`kTmdSize`) is used.

## Process teardown order issue of `score::memory::shared::MemoryResource`

The DLTv2 backend owns a trace-meta-data (TMD) shared-memory resource created via
`SharedMemoryFactory`. That resource deregisters itself from the process-wide
`score::memory::shared::MemoryResourceRegistry` singleton when it is destroyed, and
it is owned (transitively) by the lazily-created `mw::log` `Runtime` singleton.

Static/singleton objects are destroyed in reverse order of construction
completion. Because the `Runtime` (and therefore the TMD shared memory) is
typically constructed on the first logging call — i.e. *after* the shared-memory
library singletons would otherwise be built — the `MemoryResourceRegistry` could
be destroyed **before** the shared memory. The resource's destructor would then
dereference an already-destroyed registry, causing a SIGSEGV ("Memory fault") at
process exit.

TODO: To enforce the correct teardown order, the construction order must be
performed taking into consideration all other parties using `MemoryResourceRegistry`
and S-CORE project adoption

The shared-memory re-design which may fix the teardown is tracked in
[Ticket-192438](broken_link_j/Ticket-192438).

## Configuration

The recorder factory
([`dlt_trace_recorder_factory.cpp`](../../detail/dlt_trace/dlt_trace_recorder_factory.cpp))
maps the `Configuration` onto the backend constructor as follows:

| Backend input | `Configuration` getter | JSON key |
|---|---|---|
| number of slots | `GetNumberOfSlots()` | `numberOfSlots` |
| slot size (bytes) | `GetSlotSizeInBytes()` | `slotSizeBytes` |
| application description | `GetAppDescription()` | `appDesc` |
| trace shared-memory size | `GetRingBufferSize()` | `ringBufferSize` |

The `appId` and per-context log levels are applied by `DltTraceRecorder` from
the same `Configuration`. See the
[Configuration section of the `mw::log` README](../../README.md#configuration)
for the meaning of each JSON key.

### Reuse of the DataRouter configuration (not the final solution)

The trace shared-memory size passed to `CreateDltTraceLibrary()` is currently
taken from **`ringBufferSize`** (`GetRingBufferSize()`), which is the
DataRouter/DLTv1 ring-buffer field. The DLTv2 path, however, uses a **separate**
TMD shared-memory region (`/dev_dlt_tmd_`) that is independent of the DataRouter
ring buffer.

> **Note:** Reusing `ringBufferSize` for the DLTv2 trace shared-memory size is a
> temporary measure, **not the final solution**. It overloads a DataRouter-specific
> configuration field with a semantically different meaning, so sizing the TMD
> region cannot be tuned independently of the DataRouter ring buffer. A dedicated
> configuration key (e.g. `traceSharedMemorySize`) for the DLTv2 backend should
> replace this reuse. Until then, integrators must be aware that under
> `ringBufferSize` name both remote backends are controlled.

## Related documents

- [DataRouter backend (DLTv1)](./datarouter_backend/README.md) — the classic
  remote backend that DLTv2 replaces when `shm_dma_enabled=True`.
- [registry_aware_recorder_factory.md](../registry_aware_recorder_factory.md) —
  the backend plugin/registration system and Bazel targets.
- [`mw::log` README — Feature Flags](../../README.md#feature-flags) — the
  `shm_dma_enabled` build flag.
