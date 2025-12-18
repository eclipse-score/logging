# Logging configuration

The JSON-based configuration system provides predictable and manageable logging behavior.

## logging.json file

**Reference**: score/mw/log/design/configuration_design.md

### Logging Context

The logging API creates logger instances with specific context names. For example, mw::log implements this as:

```c++
CreateLogger("Ctx1").LogInfo() << "Hello, world!";
```
When logging to different contexts you are able to filter the logs coming from a particular context. It makes sense to use subsystem or component name abbreviation as context ID.
### How configuration works
- There is a list of logging contexts in the config file on the application side. You do not have to call any ```InitLogging()``` function in your code. Just use the logging API.
- The configuration file has a fixed name ```logging.json``` and resides in the directory of the adaptive application's configuration data (**/opt/&lt;AppName&gt;/etc**).
-  Developers have control over respective log output, and use [DLT Viewer](broken_link_ac/wiki/display/PSP/DLT+Viewer) or similar tools to read the logs.
### Example
Configuration file for an application (optional) is structured as follows:
```json
{
	"appId": "App1",
	"appDesc": "App1 general description",
	"logfilePath": "/tmp",
	"logLevel": "kWarn",
	"logLevelThresholdConsole": "kError",
	"logMode": "kRemote",
	"contextConfigs": [
	{
		"name": "ctx1",
		"logLevel": "kError"
	},
	{
		"name": "ctx2",
		"logLevel": "kWarn"
	},
	]
}
```
### How the translation of this file works
- application ID is set to **App1**
- logMode **kRemote** is enforced across the application
- context log level for, e.g., **ctx1**, is inferred from the following places:
  - defined in **contextConfigs** in the application defaults?
    - yes =  this value is used (kError in example), no = continue;
  - default **logLevel** for the application is defined
    - yes =  this value is used (kWarn in example), no = global default is used -- normally set by your code's **InitLogging()** call

**Note** Whichever options are **not set** the values used come from the call to ```InitLogging()``` function. If this function does not get called before actual logging, then some defaults are used, and AppId is set to PID of the process (this happens only if both no config file is provided **and** ```InitLogging()``` has not been called before)

### When do logs appear on console
This depends on the following options: ```logMode```, ```logLevelThresholdConsole```, ```logLevel```.
If ```logMode``` includes ```kConsole``` then log messages that are more severe than ```logLevelThresholdConsole``` will appear on the console. If the context is silenced or its ```logLevel``` is set to higher severity than ```logLevelThresholdConsole``` then only messages above ```logLevel``` will be appearing on the console.

### What the options are
Here is a short list of possible values and how to combine these:
- ```logLevel``` values, severity decreasing, are exclusive, you can use only one:

logLevel | Severity
---------|---------
kOff | No logging
kFatal | Fatal error, not recoverable
kError | Error with impact to correct functionality
kWarn | Warning if correct behavior cannot be ensured
kInfo | Informational, providing high level understanding
kDebug | Detailed information for programmers
kVerbose | Extra-verbose debug messages (highest detail of information)
- ```logMode```'s are combined with or (```|```) operator, e.g., ```kRemote|kConsole```, the effect is that both are used

logMode | logSplained
---------|---------
kRemote | Sent remotely to DLT daemon, and further via DLT protocol out
kFile | Save to file in a directory
kConsole | Forward to console

## log-channels.json

DLT protocol provides a possibility to define different log channels.
These channels are different in our implementation in that they use different source port, so effectively datarouter splits the DLT datastream between a number of receiving channels, based on the configuration.

This file contains the configuration of log channels. Some ECUs are required to have more than one channel for DLT logs, some have only one. This is required by the central test department, so that the amount of logs stored can be reduced.
The specific requirements as to "what has to be put where" are not yet present, but infratructure is provided.
An example file can be found in `./etc/log-channels.json`
The file has multiple sections.

- **channels** specifies log channels: with their IDs (4-byte), UDP source port, reported ECU ID
- **channelAssignments**: based on AppID and CtxID the packets can be assigned to certain log channel
- **messageThresholds**: specifies filter thresholds based on AppId and CtxId
- **defaultChannel**: if no channelAssignment has been matched, this channel is used
- **defaultThresold**: if no messageThreshold matched, this threshold is used to filter the messages
