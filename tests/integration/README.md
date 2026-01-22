# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

# Integration Tests

Integration tests for the `score_logging` component that verify DLT (Diagnostic Log and Trace) message capture on a QNX 8.0 QEMU target.

## Prerequisites

Complete the QNX 8.0 QEMU environment setup as described in the [QNX QEMU documentation](score_reference_integration/qnx_qemu/README.md).

### Network Bridge Setup (virbr0)

The integration tests use QEMU with bridge networking, which requires the `virbr0` virtual bridge interface. Instructions: reference_integration/qnx_qemu/README.md#Configure QEMU networking

This creates:
- Bridge interface `virbr0` with IP `192.168.122.1/24` on the host. Reference: /usr/share/libvirt/networks/default.xml
- DHCP server assigning IPs `192.168.122.2-254` to VMs
- NAT rules for outbound internet access

## Usage

### 1. Start the QNX QEMU Target

Build and launch the QEMU virtual machine from the `score_reference_integration/qnx_qemu` directory:

```bash
bazel build --config=x86_64-qnx \
    --credential_helper=*.qnx.com=$(pwd)/../toolchains_qnx/tools/qnx_credential_helper.py \
    //build:init

bazel run --config=x86_64-qnx //:run_qemu
```

Note the assigned IP address once the QEMU VM boots (e.g., `192.168.122.76`) since the actual IP address of the target (QEMU VM) is assigned dynamically via DHCP hence changes every reboot. We use the S_CORE_ECU_QEMU_BRIDGE_NETWORK_PP target config for the tests.

### 2. Execute the Tests

Run the integration tests from the `score_logging` directory, specifying the target IP:

```bash
bazel test //tests/integration:test_datarouter_dlt \
    --test_arg=--target-ip=<QEMU_VM_IP> \
    --test_output=streamed \
    --nocache_test_results
```

## Test Details

### test_datarouter_dlt

Validates DLT message capture from the datarouter by:

1. Connecting to the QNX target via SSH
2. Starting the datarouter process if not already running
3. Capturing DLT messages over UDP for approximately 10 seconds
4. Verifying receipt of messages with `APP_ID=DR` and `CTX_ID=STAT`

The datarouter emits statistics messages every 5 seconds. The test expects at least 1 message during the capture window for reliable test results
