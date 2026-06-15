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

"""Verify that the DataRouter log-channels.json contains a quotas configuration block."""

import json
import logging
import os
import tempfile

LOGGER = logging.getLogger(__name__)

DATAROUTER_LOG_CHANNEL_REMOTE_PATH = "/opt/datarouter/etc/log-channels.json"


def test_bandwidth_quotes_exists(target, datarouter_on_target):
    """DataRouter log-channels.json must contain a 'quotas' configuration block.

    The component shall have the DLT bandwidth quota configuration in the file
    located at /opt/datarouter/etc/log-channels.json.
    """
    with tempfile.TemporaryDirectory() as tmp_dir:
        local_path = os.path.join(tmp_dir, "log-channels.json")
        target.download(DATAROUTER_LOG_CHANNEL_REMOTE_PATH, local_path)
        LOGGER.info("Downloaded %s: %d bytes", DATAROUTER_LOG_CHANNEL_REMOTE_PATH,
                    os.path.getsize(local_path))

        with open(local_path, "r") as f:
            content = json.load(f)

        LOGGER.info("log-channels.json keys: %s", list(content.keys()))
        assert "quotas" in content, (
            f"'quotas' key missing from {DATAROUTER_LOG_CHANNEL_REMOTE_PATH}. "
            f"Found keys: {list(content.keys())}"
        )
        LOGGER.info("'quotas' block found: %s", content["quotas"])
