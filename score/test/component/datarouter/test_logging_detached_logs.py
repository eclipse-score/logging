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

"""Integration test verifying DataRouter can retrieve a client's log data
after the client exits immediately (detached mode).

The app is launched with -w 0 so it exits immediately after logging, without
waiting for DataRouter -- this forces the shared-memory writer into detached
state for DataRouter to discover. Uses kRemote + dlt_capture, since this
needs to exercise DataRouter's detached-read path specifically.

NOTE: like other dlt_capture()-based tests in this suite, this depends on
multicast DLT reception and is subject to the local Docker/multicast
limitation -- build-verified locally, relies on CI for the actual pass/fail.
"""

import logging
import time

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
DEFAULT_MESSAGE = "default message text for example log generating application"

_POST_EXIT_WAIT_SEC = 5


def test_logging_detached_logs(target, datarouter_on_target, dlt_capture):
    """Verify DataRouter retrieves logs from a client that exits immediately."""
    with dlt_capture() as receiver:
        target.execute("cd /opt/test_apps/dlt_generator && ./bin/dlt_generator -w 0")
        # Give DataRouter time to detect the detached writer and drain it.
        time.sleep(_POST_EXIT_WAIT_SEC)

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=APP_ID))
    count = sum(1 for m in messages if DEFAULT_MESSAGE in str(m.payload))
    LOGGER.info(f"Received {count} messages from the detached client")
    assert count > 0, "Couldn't find logs from dlt_generator after detached exit"
