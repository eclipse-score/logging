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
logging app's queued messages after a delayed start. Ported from SPP's
test_logging_after_delayed_dr_start.py.

SPP's assertion looks for a specific log line ("Accumulated frontend
exectution time:") that SPP's own dlt_generator prints at shutdown; SCORE's
dlt_generator (score/test/component/dlt_generator_app/dlt_generator.cpp) has
no such message. The actual intent (verify DataRouter can connect to and
drain a client's already-buffered messages once it finally starts) doesn't
depend on that specific string, so this checks for the app's regular
messages having been received instead.

Uses the datarouter_manager fixture (manual start/stop) instead of
datarouter_on_target so the app can be started BEFORE DataRouter, matching
SPP's delayed-start scenario. This needs kRemote + dlt_capture (not kFile):
the client's own local file writer doesn't care whether DataRouter is
running, so kFile-mode counting would trivially pass regardless of the
delayed-start behavior being tested -- see test_quota_exceed.py's docstring
for the same client-vs-DataRouter-side distinction. As with any
dlt_capture()-based test in this suite, this is subject to the local
Docker/multicast limitation and is build-verified locally, relying on CI for
the actual pass/fail.
"""

import logging
import time

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_DR_START_DELAY_SEC = 2
_GENERATION_ITERATIONS = 20
_GENERATION_SLEEP_MS = 500
_SHUTDOWN_WAIT_MS = 2000


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
