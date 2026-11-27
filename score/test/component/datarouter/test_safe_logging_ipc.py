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

import logging
import time

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LISI"
CTX_ID = "LOGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_FAULT_APP_CMD = "cd /opt/test_apps/fault_app && ./bin/fault_app"
_GENERATOR_CMD = (
    "cd /opt/test_apps/dlt_generator && "
    "MW_LOG_CONFIG_FILE=/opt/test_apps/fault_app/etc/logging.json "
    "nohup ./bin/dlt_generator -i 2000 > /dev/null 2>&1 & echo $!"
)
_GENERATOR_TIMEOUT_SEC = 300
_POLL_INTERVAL_SEC = 2
_POST_RUN_WAIT_SEC = 10


def _wait_for_exit(target, pid):
    deadline = time.monotonic() + _GENERATOR_TIMEOUT_SEC
    while time.monotonic() < deadline:
        exit_code, _ = target.execute(f"kill -0 {pid} 2>/dev/null")
        if exit_code != 0:
            return
        time.sleep(_POLL_INTERVAL_SEC)
    raise TimeoutError(
        f"dlt_generator (PID {pid}) did not finish within {_GENERATOR_TIMEOUT_SEC}s"
    )


def test_datarouter_when_app_disconnect_immediate(
    target, datarouter_manager, dlt_capture
):
    """Start a client that exits right after startup, then verify another client's logs still arrive."""
    datarouter_manager.start()
    exit_code, _ = target.execute(_FAULT_APP_CMD)
    assert exit_code == 0, "fault_app failed"

    with dlt_capture() as receiver:
        exit_code, out = target.execute(_GENERATOR_CMD)
        assert exit_code == 0, "Failed to start dlt_generator"
        _wait_for_exit(target, int(out.decode().split()[-1]))
        time.sleep(_POST_RUN_WAIT_SEC)

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=APP_ID, ctid=CTX_ID))
    count = sum(1 for m in messages if DEFAULT_MESSAGE in str(m.payload))
    LOGGER.info("Received %d messages from %s", count, APP_ID)
    assert count > 0, (
        f"Couldn't find logs from {APP_ID} after a client exited immediately"
    )
