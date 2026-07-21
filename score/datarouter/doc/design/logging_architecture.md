# Logging: architecture and SW design

## Definitions and abbreviations

**DLT**
Diagnostic Log and Trace is the application-level network protocol to exchange data related to logging. It has been introduced by GENIVI Alliance, and now is standardized as part of AUTOSAR Foundation.

**Logging**

1. _(generally)_: sending data items called _log messages_ from the application with the goal of development-time or production-time analysis (the specifics of _log messages_ lies in that these data is not related directly to user functions).
2. _(in more narrow context)_: sending _log messages_ in a non-structured human-readable form. Often _logging_ in narrow context means producing text files, which are formatted, but still contain strings of data or arbitrary sequences of strings and values. In this document we use term "verbose logging" to refer to this kind of information.

**Tracing**

- as opposed to logging (2): sending _log messages_ in a structured format, which is machine-readable (provided that metadata is available). In this document we use terms "non-verbose logging" and "structured logging" to refer to this kind of infomation.

## Introduction and goals

The logging framework enables software developers and problem analysts to diagnose system issues. The system extracts log messages and trace data for analysis and visualization. Since the Adaptive AUTOSAR platform lacks XCP calibration protocol support, the logging framework additionally provides a mechanism for monitoring specific values through TRACE macros.

The logging system achieves the following objectives:

1. Capture application information through well-defined interfaces
   1. Enable applications to log data efficiently.
   2. Provide uniform interfaces for system applications (Adaptive AUTOSAR stack, INT components) and userland applications (Adaptive Applications) based on standards.
   3. Minimize impact on application performance and lifecycle.
   4. Prevent blocking operations in the application.
   5. Minimize copy operations within application space.
2. Transmit logs via DLT protocol for capture by external loggers
   1. Centralize DLT message transmission configuration.
   2. Filter messages before transmission based on log level and source.
   3. Implement traffic shaping to prevent DLT data flooding.
3. Replace the missing XCP protocol on Adaptive AUTOSAR platform
   1. Transmit structured machine-readable messages.
   2. Generate metadata descriptions for structured messages.
4. Support flexible distributed configuration
   1. Implement diagnostic jobs for setting log levels per context or ECU-wide.
   2. Enable application-specific configuration.

## Constraints

The following constraints influenced the logging infrastructure design:

- System handles high data volumes in varying sizes, from single-value elements to grid fusion intermediate results
- Both calling applications and logging daemon require minimal performance overhead
- Static memory management or local allocators manage constrained memory resources
- System components rely on `mw::log` interface
- Application-side library requires safety-critical qualification

## Context

The diagram below illustrates the logging framework context:
[context-ecu](uml/context-ecu.puml)

Applications write log data through logging interfaces [^logging_vs_tracing].

The logging framework transmits data to the following sinks:

- Console Logging: Logging data to console.

- File Logging: Logging data to a file.

- Remote Logging: Logging data transmited over the netwrok using the DLT standard protocol. ECU-internal switches route UDP multicast data to the Logger, a specialized computer that captures data from multiple vehicle buses.

## Solution strategy

### Process structure

[context-highlevel](uml/context-highlevel.puml)

The logging framework implements multiple components to meet system goals:

- Application-side frontend library `mw::log`
- Central `datarouter` application that handles advanced requirements:
  - Receives data from `mw::log`
  - Formats DLT messages
  - Transmits DLT messages over different UDP ports

The `datarouter` operates as a non-safety-critical component, requiring freedom-from-interference analysis only for `mw::log` interactions.

Data serialization for DLT format occurs during write operations in `mw::log` for verbose messages. The system appends timestamps when `mw::log` processes log messages.

### Data exchange

The system decouples `mw::log` and `datarouter` by implementing communication through shared memory buffers. Connection initiation and notifications use an additional message passing channel.

The MWSR buffer structure comprises two buffers: a linear buffer and a ring buffer. The ring buffer transmits log messages while the linear buffer carries metadata and type-related dynamic serialization information. This architecture enables subscribers to access structure fields by name without knowing exact offsets or sharing code with the generating `AdaptiveApplication`.

### Structure serialization

The Serialization/Visitor pattern implements structure serialization using C++ compiler type information through multiple stages:

1. The compile-time macro:

```c++
STRUCT_TRACEABLE(S, member1, member2)
```

generates a `visit(Visitor& v, T& s)` function template that iterates through arguments and their names (`S, "member1", member1, "member2", member2`) with a compile-time `Visitor`. The visitor type serves as a function template argument, enabling `visit(v,s)` usage with different visitor implementations. Each visitor defines a `visit_struct()` entry point function.

The system defines the following visitors:
  - `serialized_visitor` - Serializes structures
  - `serialized_reflection_visitor` - Generates metainformation
  - `fibex_helper_visitor` - Generates intermediate .json files containing FIBEX generation data

2. `TRACE(S)` is a C++ **function template** (not a preprocessor macro). Its call chain at runtime is:

```c++
TRACE(arg)
  → LogEntry<T>::Instance().TryWriteIntoSharedMemory(arg)
    → SharedMemoryWriter::AllocAndWrite(serialize_fn, type_id, size)
```

On the first call for a given type `T`, `LogEntry<T>::Instance()` (a Meyers singleton) registers the type with `Logger::Instance()` and obtains a type identifier for compact serialization. Subsequent calls reuse the cached type ID. The `logging_serializer` template handles the actual serialization into the shared-memory ring buffer.

## Building block view

The diagram below illustrates the high-level class structure of the datarouter component.

[package-datarouter](uml/package-datarouter.puml)

## Runtime view

Applications access logging functionality through `mw::log`.

### Initialization

Initialization stage 1 executes when the first log request occurs. This may occur in global object constructors before the `main()` function executes, causing implicit initialization that creates necessary singletons automatically.
The activity diagram below depicts the first-run process:
[seq-trace](uml/seq-trace.puml)

### mw::log implementation

The `mw::log` implementation creates log records and commits them atomically to shared memory on flush.
[log-filtering-client-end](uml/dlt_message_filtering_frontend.puml)

### Ring buffer and linear allocator buffer

The ring buffer operates in shared memory to minimize copy overhead.
Shared memory IPC provides optimal speed and flexibility for this implementation.

### Datarouter-Client Session

The Datarouter-Client Session uses [message_passing](https://github.com/eclipse-score/communication/tree/main/score/message_passing) IPC for the initial connection, buffer acquire requests, notifications, and disconnections. The `DataRouterRecorder` sets up the session when it is created and closes it when it is destroyed. In between, the datarouter keeps one `IServerConnection` handle per client and drives the log acquisition from the server side.

The main idea is to keep clients independent, so that one non-responsive client should not stall the datarouter. This matters because the datarouter serves all clients from a single thread, one after another on each periodic tick, so a single blocking call would hold up every other client. To avoid this, the datarouter asks for buffers (to read) using non-blocking QNX pulses (`IServerConnection::Notify()`), which return right away and never wait on the client. So if a client is slow or stuck, its pulse simply stays unanswered while the healthy clients keep getting served. See [session_sequence_diagram](uml/client_session_interaction_sequence.puml) for the full flow.

However, with such a design there are risks of stale sessions. A small per-session watchdog limits how long the datarouter waits for an acquire response. If a client keeps missing its deadline, the watchdog cleans up the stale session, but first it does a best-effort read of the client's last buffer so no logs are lost unnecessarily. See [activity_watchdog_session](uml/activity_watchdog_session_lifecycle.puml).

The setup also handles an early-disconnect race, wherein if a client crashes while the datarouter is still building its session, the connect and disconnect paths work together to throw away the half-built session instead of keeping a dangling connection pointer. The client can then reconnect cleanly afterwards. See [early_disconnect_race](uml/sequence_early_disconnect_race.puml).

### datarouter

[log-filtering-datarouter](uml/dlt_message_filtering_backend.puml)

### Application-side library configuration

The system reads configuration from the `logging.json` file located in `/opt/<app>/etc`.

### datarouter configuration

The datarouter requires two configuration files:

- **log-channels.json** - Defines channel configurations, thresholds, and assignments to contexts and ECUs. Example file: [log-channels.json](../../etc/log-channels.json)

- **class-id.json** - Specifies message IDs for non-verbose DLT messages. The fibex generator produces this configuration file.

## Images

[context-ecu]: uml/context-ecu.puml "Context: logging framework in xPAD ECU (hPAD example)"
[context-highlevel]: uml/context-highlevel.puml "Implementation details: general approach"
[seq-trace]: uml/seq-trace.puml "Activity diagram for tracing functionality"
[package-datarouter]: uml/package-datarouter.puml "Package contents for datarouter"
[log-filtering-client-end]: uml/dlt_message_filtering_frontend.puml "DLT log filtering in the frontend (client side)"
[log-filtering-datarouter]: uml/dlt_message_filtering_backend.puml "DLT log filtering in the backend (Datarouter)"
