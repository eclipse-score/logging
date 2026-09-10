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

"""Integration test verifying DataRouter can pick up an already-running
logging app's queued messages after a delayed start.

Uses the datarouter_manager fixture (manual start/stop) so the app can be
started before DataRouter, then checks for the app's regular messages
having been received once DataRouter connects. Needs kRemote + dlt_capture
(not kFile): a client's local file writer doesn't care whether DataRouter
is running, so kFile-mode counting wouldn't exercise the delayed-start
behavior being tested.

NOTE: like other dlt_capture()-based tests in this suite, this depends on
multicast DLT reception and is subject to the local Docker/multicast
limitation -- build-verified locally, relies on CI for the actual pass/fail.
"""

import logging
import time

from attribute_plugin import add_test_properties
from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_DR_START_DELAY_SEC = 2
_GENERATION_ITERATIONS = 20
_GENERATION_SLEEP_MS = 500
_SHUTDOWN_WAIT_MS = 2000


@add_test_properties(
    fully_verifies=["comp_req__data_router__dlt_server"],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
def test_logging_after_delayed_dr_start(target, datarouter_manager, dlt_capture):
    """Verify DataRouter connects to and drains a client started before it."""
    with dlt_capture() as receiver:
        proc = target.execute_async(
            "/opt/test_apps/dlt_generator/bin/dlt_generator",
            args=[
                "-i",
                str(_GENERATION_ITERATIONS),
                "-s",
                str(_GENERATION_SLEEP_MS),
                "-w",
                str(_SHUTDOWN_WAIT_MS),
            ],
            cwd="/opt/test_apps/dlt_generator",
        )
        LOGGER.info(
            f"dlt_generator started; delaying DataRouter start by {_DR_START_DELAY_SEC}s"
        )
        time.sleep(_DR_START_DELAY_SEC)
        datarouter_manager.start()
        proc.wait(timeout_s=30)

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=APP_ID))
    count = sum(1 for m in messages if DEFAULT_MESSAGE in str(m.payload))
    LOGGER.info(f"Received {count} messages after delayed DataRouter start")
    assert count > 0, "No messages received after delayed DataRouter start"
