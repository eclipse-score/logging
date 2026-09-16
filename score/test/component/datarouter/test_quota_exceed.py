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

"""Integration test for datarouter's per-application bandwidth quota
enforcement.

Uses kRemote + dlt_capture rather than kFile: quota enforcement happens in
the DataRouter daemon's own read of the client's shared-memory ring buffer,
which kFile-mode writes bypass entirely.

NOTE: like other dlt_capture()-based tests in this suite, this depends on
multicast DLT reception and is subject to the local Docker/multicast
limitation -- build-verified locally, relies on CI for the actual pass/fail.

NOTE: on QNX, the default config path is baked into the read-only IFS boot
image, so each scenario's config is written to /tmp instead and datarouter
is started with --config pointing at it.
"""

import copy
import json
import logging
import os
import tempfile

from attribute_plugin import add_test_properties
from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_LINUX_LOG_CHANNELS_PATH = "/opt/datarouter/etc/log-channels.json"
_QNX_LOG_CHANNELS_PATH = "/tmp/log-channels.json"

# Sustained generation across ~30s (3 quota stats cycles @ 10s each): the
# first cycle establishes the observed rate, enforcement (if enabled) then
# applies from the second cycle onward.
_GENERATION_ITERATIONS = 3000
_GENERATION_SLEEP_MS = 10

# CheckAndSetQuotaEnforcement (data_router.cpp) computes
# rate_k_bps = totalsize_bytes * 1000 / 1024 / elapsed_ms, i.e. KB/s (not
# Kbit/s despite the "Kbps" field name in log-channels.json). Empirically
# measured baseline for the generation params above (via kFile byte count,
# same message volume applies to kRemote): ~65 KB/s. The original
# implementation's 500/100 values are calibrated to different hardware
# throughput and don't translate
# directly; these limits are chosen to sit below the measured baseline here
# so enforcement is actually exercised.

# Mirrors score/test/component/datarouter/etc/log-channels.json, varying
# only the "quotas" section per scenario.
_BASE_LOG_CHANNELS = {
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
    },
    "defaultChannel": "3493",
    "defaultThresold": "kVerbose",
    "messageThresholds": {
        "": {"vcip": "kInfo"},
        "DR": {"": "kVerbose", "CTX1": "kVerbose", "STAT": "kDebug"},
        "-NI-": {"": "kVerbose"},
    },
}

SCENARIOS = [
    {"name": "quota_disabled_50", "enabled": False, "limit_kbps": 50},
    {"name": "quota_enabled_50", "enabled": True, "limit_kbps": 50},
    {"name": "quota_enabled_15", "enabled": True, "limit_kbps": 15},
]


def _is_qnx(target) -> bool:
    exit_code, out = target.execute("uname -s")
    output = out.decode() if isinstance(out, bytes) else out
    return "QNX" in output


def _build_log_channels(enabled, limit_kbps):
    config = copy.deepcopy(_BASE_LOG_CHANNELS)
    config["quotas"] = {
        "quotaEnforcementEnabled": enabled,
        "throughput": {
            "overallMbps": 100,
            "applicationsKbps": {APP_ID: limit_kbps},
        },
    }
    return config


def _apply_scenario_config(target, enabled, limit_kbps):
    remote_path = (
        _QNX_LOG_CHANNELS_PATH if _is_qnx(target) else _LINUX_LOG_CHANNELS_PATH
    )
    config = _build_log_channels(enabled, limit_kbps)
    with tempfile.NamedTemporaryFile(mode="w", suffix=".json", delete=False) as f:
        json.dump(config, f)
        local_path = f.name
    try:
        target.upload(local_path, remote_path)
    finally:
        os.unlink(local_path)


@add_test_properties(
    fully_verifies=["comp_req__data_router__dlt_quota_bw_reduction"],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_quota_exceed(target, datarouter_manager, dlt_capture):
    """Verify quota enforcement reduces observed throughput as configured."""
    results = {}
    qnx_config_path = _QNX_LOG_CHANNELS_PATH if _is_qnx(target) else None
    for scenario in SCENARIOS:
        LOGGER.info(f"Scenario: {scenario['name']}")
        datarouter_manager.stop()
        _apply_scenario_config(target, scenario["enabled"], scenario["limit_kbps"])
        datarouter_manager.start(config_path=qnx_config_path)

        with dlt_capture() as receiver:
            target.execute(
                f"cd /opt/test_apps/dlt_generator && "
                f"./bin/dlt_generator -i {_GENERATION_ITERATIONS} -s {_GENERATION_SLEEP_MS}"
            )

        record = download_dlt(target, receiver.dlt_file)
        messages = record.find(query=dict(apid=APP_ID))
        count = sum(1 for m in messages if DEFAULT_MESSAGE in str(m.payload))
        LOGGER.info(f"{scenario['name']}: {count} messages received")
        results[scenario["name"]] = count

    LOGGER.info(f"Results: {results}")
    assert results["quota_enabled_50"] <= results["quota_disabled_50"], (
        f"50 enabled ({results['quota_enabled_50']}) should be <= disabled "
        f"({results['quota_disabled_50']})"
    )
    assert results["quota_enabled_15"] < results["quota_enabled_50"], (
        f"15 ({results['quota_enabled_15']}) should be < 50 "
        f"({results['quota_enabled_50']})"
    )
