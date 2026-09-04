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
enforcement. Ported from SPP's test_quota_exceed.py.

Quota enforcement is re-evaluated every 10s (score/datarouter/src/daemon/
socketserver.cpp: kStatisticsLogPeriodUs = 10_000_000), matching SPP's own
10s ShowStats() cycle. Messages exceeding the app's applicationsKbps limit
are dropped inside the DataRouter daemon's SourceSession read loop
(data_router.cpp: ProcessAndRouteLogMessages -> on_new_record returns early
when quota_overlimit_detected).

Important: this drop happens in the DataRouter DAEMON's own read of the
client's shared-memory ring buffer -- it is specific to the kRemote path.
kFile-mode messages are written directly by the client process via mw::log's
own FileRecorder backend, entirely bypassing DataRouter's SourceSession, so
they are NOT subject to quota enforcement at all (confirmed empirically:
kFile-based counting showed identical message counts across every quota
scenario, enabled or not). This test therefore uses kRemote + dlt_capture to
observe the actually-throttled message stream, unlike the kFile-based
counting used elsewhere in this test suite -- which also means, unlike most
of this suite, it depends on multicast DLT reception and is subject to the
same local Docker/multicast limitation as test_filetransfer/
test_datarouter_filters: build-verified locally, relies on CI for the actual
pass/fail.

Each scenario writes its own log-channels.json to the target (via
target.upload, replacing the file datarouter reads its quota config from)
and restarts datarouter via the datarouter_manager fixture to pick it up --
this uses only datarouter's existing config-reload-on-restart behavior, not
any new capability.
"""

import copy
import json
import logging
import os
import tempfile

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_LINUX_LOG_CHANNELS_PATH = "/opt/datarouter/etc/log-channels.json"
_QNX_LOG_CHANNELS_PATH = "/usr/bin/datarouter/etc/log-channels.json"

# Sustained generation across ~30s (3 quota stats cycles @ 10s each): the
# first cycle establishes the observed rate, enforcement (if enabled) then
# applies from the second cycle onward.
_GENERATION_ITERATIONS = 3000
_GENERATION_SLEEP_MS = 10

# CheckAndSetQuotaEnforcement (data_router.cpp) computes
# rate_k_bps = totalsize_bytes * 1000 / 1024 / elapsed_ms, i.e. KB/s (not
# Kbit/s despite the "Kbps" field name in log-channels.json). Empirically
# measured baseline for the generation params above (via kFile byte count,
# same message volume applies to kRemote): ~65 KB/s. SPP's own 500/100
# values are calibrated to SPP's hardware throughput and don't translate
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


def test_quota_exceed(target, datarouter_manager, dlt_capture):
    """Verify quota enforcement reduces observed throughput as configured."""
    results = {}
    for scenario in SCENARIOS:
        LOGGER.info(f"Scenario: {scenario['name']}")
        datarouter_manager.stop()
        _apply_scenario_config(target, scenario["enabled"], scenario["limit_kbps"])
        datarouter_manager.start()

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
