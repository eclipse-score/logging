# Logging Framework

This document provides an overview of the platform logging framework components that implement remote DLT (Diagnostic Log and Trace) logging capabilities. The framework enables efficient message routing from multiple sources to remote clients over a network.

For detailed information, refer to:
- **Usage Guidelines**: [./doc/guideline](./doc/guideline) - Configuration and integration instructions
- **Design Documentation**: [./doc/design](./doc/design) - Architecture and implementation details

## Sources

A source represents an application that sends logging messages to the datarouter. Sources use the `mw::log` library to transmit messages.

Applications configured with remote sinks establish a connection to the datarouter using a control channel. For message transmission, sources write messages into a ring buffer located in shared memory, while the datarouter manages buffer access by locking sections on the source side.

## Datarouter

The datarouter application routes log messages from source applications to external network clients outside the ECU. It implements core logging functionalities, including the DLT daemon interface.

### Statistics

The datarouter collects basic statistics for all source sessions. The system transmits these statistics under the `STAT` context ID and includes the following metrics for each source:
- **count** - Total number of messages received
- **size** - Cumulative payload size in bytes
- **read_time** - Time interval (monotonic clock) for the datarouter thread to read and process all messages
- **transp_delay** - Maximum time interval (monotonic clock) between message creation and datarouter read operation
- **time_between_to_calls_** - Time difference between the previous processing cycle and the current message processing
- **time_to_process_records_** - Duration required to process the current message set
- **buffer size watermark** - Maximum occupied space in the ring buffer since application start
- **messages dropped** - Cumulative count of dropped messages since start

Example statistics message:
```
[APP1 : count  4074 , size  714378  B, read_time: 21000  us, transp_delay: 204000  us  time_between_to_calls_: 0  us  time_to_process_records_: 0  us  buffer size watermark:  152  KB out of 512  KB ( 29 %)  messages dropped:  0  (accumulated)]
```
This example shows source `APP1` transmitted 4074 messages with a total payload size of 714378 bytes. The datarouter spent 204000 microseconds reading the messages (since the last read cycle).

The datarouter detects message gaps in the incoming flow and reports them using the source ID as the context ID.

Example gap detection message:
```
535575 2019/11/05 14:59:22.720133 244.3038 157 ECU1 DR 485 0 log error verbose 5 message drop detected: messages 1073 to 1110 lost!
```
The source ID corresponds to its PID. In this example, the source with PID `485` lost 37 messages because the datarouter did not read the ring buffer fast enough.

## Configuration

Configure logging to match application requirements through static configuration during application deployment. Deploy the configuration file relative to the binary under the `etc` directory (e.g., `./etc/logging.json`).

The following configuration parameters control logging performance (example for `datarouter`):
- **"appId"** - Four-character application identifier
- **"appDesc"** - Application description
- **"logFilePath"** - Directory path where the system writes the `appId.dlt` logfile when `logMode` includes `kFile`
- **"logLevel"** - Global log level threshold for the application
- **"logLevelThresholdConsole"** - Log level filter for console messages when `logMode` includes `kConsole` (global threshold takes priority)
- **"logMode"** - Logging destination combination: `kFile`, `kConsole`, `kRemote`, `kSystem`, or `kOff` to disable logging
- **"contextConfigs"** - List of individual logging context configurations
- **"stackBufferSize"** - Linear buffer size for storing metadata (increase for applications with many different data types)
- **"ringBufferSize"** - Ring buffer size for data sent to datarouter (includes all verbose/non-verbose DLT messages and CDC; increase to prevent message loss)
- **"overwriteOnFull"** - Ring buffer write strategy: `true` (default) overwrites oldest data when full; `false` drops new messages while preserving oldest unread messages (useful for identifying buffer overflow scenarios)
