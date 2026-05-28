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

"""Integration tests for mw::log kFile (DLT file) logging.
Verifies that the score_logging library correctly writes DLT messages to a
kFile log (/tmp/LGGG.dlt) with the expected message count when using the
dlt_generator application.  Tests both single-threaded and multi-threaded
scenarios.
"""

import logging
import os
import tempfile

import python_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
DEFAULT_MESSAGE = "default message text for example log generating application"
# Number of log messages generated per iteration when use_full_output=true
MESSAGES_PER_ITERATION = 6

ITERATIONS = 100
THREADS = 4


def _verify_kfile_messages(target, expected_count):
    """Download /tmp/LGGG.dlt from target and verify expected message count."""
    # Wait until the dlt file exists on the target
    exit_code, _ = target.execute("test -f /tmp/LGGG.dlt && echo EXISTS")
    assert exit_code == 0, "DLT kFile /tmp/LGGG.dlt does not exist on target"

    local_dir = tempfile.mkdtemp(prefix="dlt_kfile_")
    local_path = os.path.join(local_dir, "LGGG.dlt")
    target.download("/tmp/LGGG.dlt", local_path)
    LOGGER.info(f"Downloaded LGGG.dlt: {os.path.getsize(local_path)} bytes")

    messages = python_dlt.load(local_path)
    count = 0
    for msg in messages:
        try:
            apid = getattr(msg, "apid", None)
            if apid is None:
                continue
            if isinstance(apid, bytes):
                apid = apid.decode("utf-8", errors="replace").rstrip("\x00")
            if apid != APP_ID:
                continue
            payload = msg.payload_decoded
            if isinstance(payload, bytes):
                payload = payload.decode("utf-8", errors="replace")
            if DEFAULT_MESSAGE in payload:
                count += 1
        except Exception:
            pass

    LOGGER.info(f"Message count: {count}, expected: {expected_count}")
    assert count == expected_count, (
        f"Expected {expected_count} messages in kFile, got {count}"
    )


def test_mw_log_kfile(target, datarouter_on_target):
    """Verify DLT kFile logging with single thread and 100 iterations.
    Expected: 100 iterations × 6 messages/iteration = 600 messages.
    """
    # Remove any leftover dlt file from previous test runs
    target.execute("rm -f /tmp/LGGG.dlt")

    target.execute(
        f"cd /opt/test_apps/dlt_generator && "
        f"./bin/dlt_generator -i {ITERATIONS} -s 0 -c true -f true -w 500"
    )

    expected = ITERATIONS * MESSAGES_PER_ITERATION
    _verify_kfile_messages(target, expected)
    LOGGER.info(f"test_mw_log_kfile passed: {expected} messages verified")


def test_mw_log_kfile_multiple_threads(target, datarouter_on_target):
    """Verify DLT kFile logging with 4 threads and 100 iterations each.
    Expected: 4 threads × 100 iterations × 6 messages/iteration = 2400 messages.
    """
    # Remove any leftover dlt file from previous test runs
    target.execute("rm -f /tmp/LGGG.dlt")

    target.execute(
        f"cd /opt/test_apps/dlt_generator && "
        f"./bin/dlt_generator -t {THREADS} -i {ITERATIONS} -s 0 -c true -f true -w 500"
    )

    expected = THREADS * ITERATIONS * MESSAGES_PER_ITERATION
    _verify_kfile_messages(target, expected)
    LOGGER.info(f"test_mw_log_kfile_multiple_threads passed: {expected} messages verified")
