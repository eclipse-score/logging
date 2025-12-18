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
- System components rely on `ara::log` or `mw::log` interfaces requiring backward compatibility
- Application-side library requires safety-critical qualification

## Context

The diagram below illustrates the logging framework context:
![alt text][context-ecu]

Applications write log data through logging interfaces [^logging_vs_tracing].

The logging framework transmits data to the following sinks:

- Console Logging: Logging data to console.

- File Logging: Logging data to a file.

- Remote Logging: Logging data transmited over the netwrok using the DLT standard protocol. ECU-internal switches route UDP multicast data to the Logger, a specialized computer that captures data from multiple vehicle buses.

## Solution strategy

### Process structure

![alt text][context-highlevel]

The logging framework implements multiple components to meet system goals:

- Application-side frontend library `mw::log`
- Central `datarouter` application that handles advanced requirements:
  - Receives data from `mw::log`
  - Formats DLT messages
  - Transmits DLT messages over different UDP ports

The `datarouter` operates as a non-safety-critical component, requiring freedom-from-interference analysis only for `mw::log` interactions.

Data serialization for DLT format occurs during write operations in `mw::log` or `ara::log` interface for verbose messages. The system appends timestamps when `mw::log` processes log messages.

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

2. The `TRACE(S)` macro expands at compile time to:

```c++
log_entry<type(S)>::instance() = S
```

This construct uses the `log_entry` singleton template to register type information in the logger once, then employs the type ID for serialization. The `serializer<allocator, T>` template writes the serialized structure representation while the `log_entry_allocator` allocates space in the ring buffer.

## Building block view

The diagrams below illustrate the high-level class structure of logging framework components.

The [ara::log][1] implementation conforms to Adaptive AUTOSAR specification R1903.
![alt text][package-ara-log]

![alt text][package-datarouter]

## Runtime view

Applications access logging functionality either through `mw::log` directly or through the adpative AUTOSAR standardized `ara::log` interface.

### Initialization

The `ara::log` runtime depends entirely on `mw::log` functionality. Using `ara::log` implicitly triggers library initialization.

Initialization stage 1 executes when the first log request occurs. This may occur in global object constructors before the `main()` function executes, causing implicit initialization that creates necessary singletons automatically.
The activity diagram below depicts the first-run process:
![alt text][seq-trace]

### ara::log implementation

The Adaptive AUTOSAR logging interface implementation follows standard specifications. The system creates LogStream objects dynamically to enable isolated collection of log message items and atomic message commits on stream flush.
![alt text][seq-ara-log]
![alt text][log-filtering-client-end]

### Ring buffer and linear allocator buffer

The ring buffer operates in shared memory to minimize copy overhead.
Shared memory IPC provides optimal speed and flexibility for this implementation.

### Message passing interaction

The message passing connection transmits initial connection information, notifications, and handles disconnections. The connection lifecycle proceeds as follows:

- The `DataRouterRecorder` constructor creates a `DatarouterMessageClient` instance, spawning a thread that attempts server connections every 100ms.
- Upon successful connection, the client transmits a message containing application information to the server.
- The `DataRouterRecorder` destructor notifies the thread, causing normal termination.

### datarouter

![alt text][log-filtering-datarouter]

### Application-side library configuration

The system reads configuration from the `logging.json` file located in `/opt/<app>/etc`.

### datarouter configuration

The datarouter requires two configuration files:

- **log-channels.json** - Defines channel configurations, thresholds, and assignments to contexts and ECUs. Example file: [log-channels.json](../../etc/log-channels.json)

- **class-id.json** - Specifies message IDs for non-verbose DLT messages. The fibex generator produces this configuration file.

## Images

[context-ecu]: uml/context-ecu.png "Context: logging framework in xPAD ECU (hPAD example)"
[context-highlevel]: uml/context-highlevel.png "Implementation details: general approach"
[seq-ara-log]: uml/seq-ara-log.png "ara::log call conversion to libtracing"
[seq-trace]: uml/seq-trace.png "Activity diagram for tracing functionality"
[package-ara-log]: uml/package-ara-log.png "Package contents for ara::log"
[package-datarouter]: uml/package-datarouter.png "Package contents for datarouter"
[log-filtering-client-end]: uml/dlt_message_filtering_frontend.png "DLT log filtering in the frontend (client side)"
[log-filtering-datarouter]: uml/dlt_message_filtering_backend.png "DLT log filtering in the backend (Datarouter)"

## References

[1]: http://example.com/  "SWS_Log"
[2]: http://example.com/ "DLT Protocol"
