# dlt_stats

`dlt_stats` evaluates DLT throughput statistics from a DLT or PCAP trace.

**Usage**:

The tool supports DLT files and PCAP/PCAPNG traces as an input.
By default ECUID `MPCH` is used.

```bash
bazel run //platform/aas/test/pas/datarouter/dlt_stats -- --ecuid=MPCH --pcap_trace <absolute file path to .pcap file> > dlt_stats.json
```

```bash
bazel run //platform/aas/test/pas/datarouter/dlt_stats -- --ecuid=MPCH --dlt_trace <absolute file path to .dlt file> > dlt_stats.json
```

If not trace file is provided, the tool attempts to capture a trace locally using tcpdump:

```bash
bazel run //platform/aas/test/pas/datarouter/dlt_stats -- --ecuid=MPCH > dlt_stats.json
```

The output json contains the average DLT payload throughput in KB/sec.
In case of a verbose message, the key is `<APPID>,<CTXID>`.
For non-verbose messages the key is the message ID, as specified in FIBEX.
Fixed IDs can also be looked up e.g. [here](broken_link_g/swh/abc-lmn/blob/master/config/common/pas/logging/application-ids.json).
