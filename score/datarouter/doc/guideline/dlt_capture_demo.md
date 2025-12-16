# Capturing DLT Logs

This guide configures a minimal setup for capturing DLT logs on a loopback interface. The `demo_app` application uses `mw::log` to generate log messages, while the `datarouter` daemon transmits these messages as UDP packets over the network for capture on the loopback interface.

## Prerequisites

### Software Requirements

- **DLT capture tool** (choose one):
  - [dlt-viewer](https://github.com/COVESA/dlt-viewer) - GUI-based DLT log viewer
  - [dlt-receive](https://github.com/COVESA/dlt-daemon/blob/master/doc/dlt-receive.1.md) - Command-line DLT receiver
  - [Wireshark](https://www.wireshark.org/) - Network protocol analyzer with DLT support

### System Requirements

- x86 target platform
- Network loopback interface (127.0.0.1) configured and operational as a multicast interface.
- UDP port 3490 available for DLT traffic

## Datarouter Setup

### Installation Structure

Deploy the datarouter daemon with the following directory structure:

```
/opt/datarouter/
├── bin/
│   └── datarouter
└── etc/
    ├── logging.json
    └── log-channels.json
```

**Note**: Reference configuration files are available in the `./etc` directory.

### Starting the Datarouter

Execute the datarouter daemon on x86 targets:

```bash
./datarouter --no_adaptive_runtime &
```

The `--no_adaptive_runtime` flag bypasses Adaptive runtime requirements for standalone operation.

## Demo Application Setup

### Installation Structure

Deploy the demo application with the following structure:

```
/opt/demo_app/
├── bin/
│   └── demo_app
└── etc/
    └── logging.json
```

### Running the Application

Execute the demo application to generate log messages:

```bash
./demo_app
```

Assunption is that the application uses `mw::log` to log structured debug log messages.

## Capturing DLT Logs

### Option 1: Using dlt-receive (Command-Line)

Capture DLT messages on the loopback interface using the command-line tool:

```bash
./dlt-receive --udp --net-if 127.0.0.1 --port 3490 -a
```

**Parameters:**
- `--udp`: Enable UDP reception mode
- `--net-if 127.0.0.1`: Bind to loopback interface
- `--port 3490`: Listen on default DLT UDP port
- `-a`: Display all messages in ASCII format

### Option 2: Using DLT Viewer (GUI)

1. Launch [dlt-viewer](https://github.com/COVESA/dlt-viewer)
2. Configure UDP connection:
   - Interface: `127.0.0.1`
   - Port: `3490`
3. Start capture to view real-time log messages

### Option 3: Using Wireshark

1. Launch [Wireshark](https://www.wireshark.org/)
2. Navigate to **Capture** menu
3. Select **Adapter for loopback traffic capture**
4. Apply display filter: `udp.port == 3490`
5. Start capture to monitor raw DLT UDP packets

## Verification

Verify the setup operates correctly by checking:

1. **Datarouter process**: Confirm the daemon runs without errors
   ```bash
   ps aux | grep datarouter
   ```

2. **Network binding**: Verify UDP port 3490 listens on loopback
   ```bash
   netstat -uln | grep 3490
   ```

3. **Log output**: Observe DLT messages appear in the capture tool when executing `demo_app`

## Troubleshooting

### No DLT messages received

- Verify datarouter daemon runs with `--no_adaptive_runtime` flag
- Check UDP port 3490 availability: `netstat -uln | grep 3490`
- Confirm loopback interface configuration: `ip addr show lo`
- Validate configuration files in `/opt/datarouter/etc/`

### Configuration file errors

- Verify JSON syntax in `logging.json` and `log-channels.json`
- Check file permissions allow read access for datarouter process
- Review datarouter logs for configuration parsing errors
