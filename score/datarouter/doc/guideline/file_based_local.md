# How to configure logging into a file

You sometimes want to get a log in the filesystem. No matter if qemu or normal, this should work. Though you can set it up at application level only, it can bring significant benefits for offline analysis.

1. take a look at the following document regarding configuration of logging: [xyz doc](broken_link_g/xyz/documentation/blob/master/guidelines/logging/configuration.md)

The following config will set App1 to write into /tmp/App1.dlt

```json
{
  "appId": "App1",
  "appDesc": "App1 general description",
  "logfilePath": "/tmp",
  "logLevel": "kWarn",
  "logMode": "kRemote|kFile",
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

2. Specifically,
- have a ```logging.json``` file. It is very useful at the moment.
- in your ```logging.json``` file append **```|kFile```** to your _logMode_,
and as _logfilePath_ use the target <u>directory</u>, e.g. "**/tmp**".

