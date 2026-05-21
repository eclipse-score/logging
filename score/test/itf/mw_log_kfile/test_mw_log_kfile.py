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

"""Integration test for mw::log kFile logging mode.

Verifies that dlt_generator binary correctly writes DLT messages to a local
file when configured with kFile|kRemote log mode. Message counts are asserted
against the on-disk .dlt file (written directly by the datarouter from shared
memory). Remote DLT output via UDP is logged for diagnostics but not asserted
exactly, because UDP multicast can truncate messages under load.
"""

import logging
import os
import tempfile

import dlt.dlt as python_dlt

from dlt_parser import parse_messages


LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
ITERATIONS = 100
DEFAULT_MESSAGE = "default message text for example log generating application"
# With use_full_output=true and kVerbose log level via remote:
# Fatal + Error + Warn + Info + Verbose + Debug = 6 messages per iteration
MESSAGES_PER_ITERATION = 6


def _verify_kfile_messages(target, expected):
    """Download /tmp/LGGG.dlt from the target and assert the message count."""
    exit_code, _ = target.execute("test -f /tmp/LGGG.dlt && echo EXISTS")
    assert exit_code == 0, "DLT file /tmp/LGGG.dlt was not created on the target"

    with tempfile.TemporaryDirectory() as tmpdir:
        local_dlt = os.path.join(tmpdir, "LGGG.dlt")
        target.download("/tmp/LGGG.dlt", local_dlt)
        LOGGER.info(f"Downloaded DLT file: {os.path.getsize(local_dlt)} bytes")

        dlt_messages = python_dlt.load(local_dlt, None)
        file_count = sum(
            1
            for m in dlt_messages
            if getattr(m, "apid", None) == APP_ID
            and DEFAULT_MESSAGE in str(getattr(m, "payload_decoded", ""))
        )
        LOGGER.info(f"DLT file message count: {file_count}")
        assert file_count == expected, (
            f"DLT file: expected {expected} messages, got {file_count}"
        )


def test_mw_log_kfile(target, datarouter_on_target, dlt_capture):
    """Verify kFile logging produces the expected number of messages.

    Runs dlt_generator with 100 iterations, each producing 6 log messages
    (Fatal, Error, Warn, Info, Verbose, Debug). Verifies the exact count
    from the on-disk .dlt file. Remote DLT output via UDP is logged for
    diagnostics but not asserted exactly.
    """
    expected = ITERATIONS * MESSAGES_PER_ITERATION

    with dlt_capture() as receiver:
        target.execute(
            "cd /opt/test_apps/dlt_generator && ./bin/dlt_generator"
            f" -i {ITERATIONS} -s 0 -c true -w 500"
        )

    # Log remote DLT count for diagnostics (UDP is inherently lossy)
    output = receiver.get_output()
    messages = parse_messages(output, APP_ID)
    remote_count = sum(1 for m in messages if DEFAULT_MESSAGE in m.payload)
    LOGGER.info(f"Remote DLT message count: {remote_count} / {expected} expected")

    _verify_kfile_messages(target, expected)


def test_mw_log_kfile_multiple_threads(target, datarouter_on_target, dlt_capture):
    """Verify kFile logging with multiple threads.

    Runs dlt_generator with 4 threads and 100 iterations each, producing
    4 * 6 * 100 = 2400 messages total. Verifies the exact count from the
    on-disk .dlt file (written directly by the datarouter from shared memory).

    Remote DLT output via UDP is logged for diagnostics but not asserted
    exactly, because UDP multicast can truncate or drop messages under
    concurrent load — this is inherent to the protocol, not a bug.
    """
    threads = 4
    expected = threads * ITERATIONS * MESSAGES_PER_ITERATION

    with dlt_capture() as receiver:
        target.execute(
            "cd /opt/test_apps/dlt_generator && ./bin/dlt_generator"
            f" -t {threads} -i {ITERATIONS} -s 0 -c true -w 500"
        )

    # Log remote DLT count for diagnostics (UDP is inherently lossy)
    output = receiver.get_output()
    messages = parse_messages(output, APP_ID)
    remote_count = sum(1 for m in messages if DEFAULT_MESSAGE in m.payload)
    LOGGER.info(f"Remote DLT message count: {remote_count} / {expected} expected")

    _verify_kfile_messages(target, expected)
