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
after the client exits immediately (detached mode). Ported from SPP's
test_logging_detached_logs.py.

Product support for this is confirmed present: score/mw/log/detail/
data_router/shared_memory/shared_memory_reader.{h,cpp} implements
ReadDetached()/DetachWriter()/IsWriterDetached(), and
score/datarouter/datarouter/data_router.cpp's
SourceSession::ProcessDetachedLogs() drives it from the daemon side.

SPP's version uses logMode "kSystem" and asserts on a specific dlt_generator
shutdown log line ("Accumulated frontend exectution time:") that doesn't
exist in SCORE's dlt_generator (score/test/component/dlt_generator_app/
dlt_generator.cpp). "kSystem" isn't a DataRouter-relay mode in SCORE (it
maps to the QNX slog2 backend, see score/mw/log/backend/slog_registrant.cpp)
-- this test needs the kRemote path specifically to exercise DataRouter's
detached-read code at all, so it uses the existing dlt_generator_filesystem
(kFile|kRemote) instead, and asserts on the app's regular messages having
been received rather than a message SCORE's app doesn't print.

The app is launched with -w 0 (sleep_before_shutdown_ms=0) so it exits
immediately after logging, without waiting for DataRouter -- this is what
forces the shared-memory writer into detached state for DataRouter to
discover. Needs kRemote + dlt_capture (see test_quota_exceed.py's docstring
for why kFile-mode counting wouldn't exercise the DataRouter-side behavior
being tested here); subject to the same local Docker/multicast limitation
as other dlt_capture()-based tests in this suite -- build-verified locally,
relies on CI for the actual pass/fail.
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
