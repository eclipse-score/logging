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

"""DataRouter robustness tests: detached-app logging and delayed DR start."""

import logging
import time

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_DETACHED_FLUSH_WAIT_S = 5
_DR_START_DELAY_S = 2
_DR_FLUSH_WAIT_S = 5
_APP_HOLD_AFTER_LOG_MS = 8000


def test_logging_detached_logs(target, datarouter_on_target, dlt_capture):
    """DataRouter must recover logs from an app that exits before DR syncs."""
    with dlt_capture() as receiver:
        target.execute(
            "cd /opt/test_apps/dlt_generator && ./bin/dlt_generator"
            " -i 10 -s 0 -f true -w 0"
        )
        time.sleep(_DETACHED_FLUSH_WAIT_S)

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=APP_ID))
    LOGGER.info("Received %d messages from detached app", len(messages))

    assert len(messages) > 0, "DataRouter failed to recover detached app logs"
    payloads = "\n".join(str(m.payload) for m in messages)
    assert DEFAULT_MESSAGE in payloads, "Expected log content not found"


def test_logging_after_delayed_dr_start(target, datarouter_manager, dlt_capture):
    """Messages buffered in shared memory must not be lost when DR starts late."""
    with dlt_capture() as receiver:
        proc = target.execute_async(
            "/opt/test_apps/dlt_generator/bin/dlt_generator",
            args=[
                "-i",
                "10",
                "-s",
                "0",
                "-f",
                "true",
                "-w",
                str(_APP_HOLD_AFTER_LOG_MS),
            ],
            cwd="/opt/test_apps/dlt_generator",
        )
        time.sleep(_DR_START_DELAY_S)
        datarouter_manager.start()
        proc.wait(timeout_s=30)
        time.sleep(_DR_FLUSH_WAIT_S)

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=APP_ID))
    LOGGER.info("Received %d messages after delayed DR start", len(messages))

    assert len(messages) > 0, "Messages logged before DR started were lost"
    payloads = "\n".join(str(m.payload) for m in messages)
    assert DEFAULT_MESSAGE in payloads, "Expected log content not found"
