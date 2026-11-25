# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
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

import collections
import json
import logging
import os
import tempfile

from attribute_plugin import add_test_properties
from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
CTX_ID = "LOGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_LINUX_LOG_CHANNELS_PATH = "/opt/datarouter/etc/log-channels.json"
_QNX_LOG_CHANNELS_PATH = "/tmp/log-channels.json"

_GENERATION_ITERATIONS = 50
_GENERATION_SLEEP_MS = 20

_LOG_CHANNELS = {
    "channels": {
        "3491": {
            "address": "0.0.0.0",
            "channelThreshold": "kError",
            "dstAddress": "239.255.42.99",
            "dstPort": 3490,
            "ecu": "TST1",
            "port": 3491,
        },
        "3492": {
            "address": "0.0.0.0",
            "channelThreshold": "kInfo",
            "dstAddress": "239.255.42.99",
            "dstPort": 3490,
            "ecu": "TST2",
            "port": 3492,
        },
        "3493": {
            "address": "0.0.0.0",
            "channelThreshold": "kVerbose",
            "dstAddress": "239.255.42.99",
            "dstPort": 3490,
            "ecu": "TST3",
            "port": 3493,
        },
    },
    "channelAssignments": {
        "DR": {"": ["3492"], "CTX1": ["3492", "3493"]},
        "-NI-": {"": ["3491"]},
        APP_ID: {CTX_ID: ["3491", "3492", "3493"]},
    },
    "defaultChannel": "3493",
    "defaultThresold": "kVerbose",
    "messageThresholds": {
        "": {"vcip": "kInfo"},
        "DR": {"": "kVerbose", "CTX1": "kVerbose", "STAT": "kDebug"},
        "-NI-": {"": "kVerbose"},
    },
}

_MESSAGES_PER_ITERATION_BY_ECU = {"TST1": 2, "TST2": 4, "TST3": 6}


def _is_qnx(target) -> bool:
    exit_code, out = target.execute("uname -s")
    output = out.decode() if isinstance(out, bytes) else out
    return "QNX" in output


def _upload_log_channels(target, remote_path):
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
        json.dump(_LOG_CHANNELS, f)
        local_path = f.name
    try:
        target.upload(local_path, remote_path)
    finally:
        os.unlink(local_path)


@add_test_properties(
    fully_verifies=[
        "comp_req__data_router__dlt_multiple_channels",
        "comp_req__data_router__dlt_channel_config",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_dlt_multiple_channels(target, datarouter_manager, dlt_capture):
    """Verify each configured DLT channel applies its own log level threshold."""
    qnx = _is_qnx(target)
    datarouter_manager.stop()
    _upload_log_channels(
        target, _QNX_LOG_CHANNELS_PATH if qnx else _LINUX_LOG_CHANNELS_PATH
    )
    datarouter_manager.start(config_path=_QNX_LOG_CHANNELS_PATH if qnx else None)

    with dlt_capture() as receiver:
        target.execute(
            f"cd /opt/test_apps/dlt_generator && "
            f"./bin/dlt_generator -i {_GENERATION_ITERATIONS} -s {_GENERATION_SLEEP_MS}"
        )

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=APP_ID, ctid=CTX_ID))
    counts = collections.Counter(
        m.raw_msg.ecuid for m in messages if DEFAULT_MESSAGE in str(m.payload)
    )
    LOGGER.info(f"Messages per ECU ID: {dict(counts)}")

    expected = {
        ecu: per_iteration * _GENERATION_ITERATIONS
        for ecu, per_iteration in _MESSAGES_PER_ITERATION_BY_ECU.items()
    }
    assert dict(counts) == expected, (
        f"Expected messages per ECU ID {expected}, got {dict(counts)}"
    )
