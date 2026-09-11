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

"""Integration test verifying datarouter's log-channels.json declares DLT
bandwidth quotas.
"""

import json
import logging
import os
import tempfile

from attribute_plugin import add_test_properties

LOGGER = logging.getLogger(__name__)

_LINUX_LOG_CHANNELS_PATH = "/opt/datarouter/etc/log-channels.json"
_QNX_LOG_CHANNELS_PATH = "/usr/bin/datarouter/etc/log-channels.json"


def _is_qnx(target) -> bool:
    exit_code, out = target.execute("uname -s")
    output = out.decode() if isinstance(out, bytes) else out
    return "QNX" in output


@add_test_properties(
    fully_verifies=["comp_req__data_router__dlt_bw_quota_config"],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_bandwidth_quotes_exists(target):
    """Verify datarouter's log-channels.json declares a 'quotas' section."""
    remote_path = (
        _QNX_LOG_CHANNELS_PATH if _is_qnx(target) else _LINUX_LOG_CHANNELS_PATH
    )

    with tempfile.TemporaryDirectory() as tmpdir:
        local_path = os.path.join(tmpdir, "log-channels.json")
        target.download(remote_path, local_path)

        with open(local_path, "r") as f:
            log_channel_content = json.load(f)

        LOGGER.info(f"log-channels.json keys: {list(log_channel_content.keys())}")
        assert "quotas" in log_channel_content
