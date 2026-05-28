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

"""Integration tests for mw::log remote, console, and file logging.
Verifies that the score_logging library correctly logs all supported data types
in all three logging modes: remote (DLT over UDP), console (stdout), and file
(DLT file on the target).
NOTE: UINT64_MAX renders as -1 in remote DLT output (dlt-receive text format)
      but as 18446744073709551615 in console/file output.
"""

import logging
import tempfile
import os

import pytest
import python_dlt

from dlt_parser import count_messages_by_context

LOGGER = logging.getLogger(__name__)

APP_ID = "EXA"

# Values to check in remote DLT text output
VALUES_TO_CHECK_REMOTE = [
    "true",
    "false",
    "255",
    "65535",
    "4294967295",
    "-1",          # UINT64_MAX renders as -1 in dlt-receive text output
    "-128",
    "-32768",
    "-2147483648",
    str(-(2**63)),
    "0",
    "127",
    "32767",
    "2147483647",
    str(2**63 - 1),
    "test string",
    "3.14",
    "0xab",
    "0xabcd",
    "0xabcdef01",
    "0xabcdef0123456789",
    "0b10101010",
    "0b1010101010101010",
    "0b10101010101010101010101010101010",
    "0b1010101010101010101010101010101010101010101010101010101010101010",
    "raw",
    "slog2_message",
]

# Values to check in console/file output (UINT64_MAX shown as full number)
VALUES_TO_CHECK_CONSOLE_FILE = [v for v in VALUES_TO_CHECK_REMOTE if v != "-1"] + [
    "18446744073709551615"
]


def test_mw_log_remote_logging(target, datarouter_on_target, dlt_capture):
    """Verify all supported data types appear in DLT remote (UDP) output."""
    with dlt_capture() as receiver:
        target.execute("cd /opt/test_apps/logging_app && ./bin/logging_app")

    output = receiver.get_output()
    LOGGER.info(f"DLT remote output length: {len(output)} chars")

    missing = []
    for value in VALUES_TO_CHECK_REMOTE:
        if value not in output:
            missing.append(value)

    assert not missing, f"Missing values in remote DLT output: {missing}"
    LOGGER.info("Remote logging test passed")


def test_mw_log_console_logging(target, datarouter_on_target):
    """Verify all supported data types appear in console (stdout) output."""
    proc = target.execute_async(
        "/opt/test_apps/logging_app/bin/logging_app",
    )
    proc.wait()
    output = proc.get_output()
    LOGGER.info(f"Console output length: {len(output)} chars")

    missing = []
    for value in VALUES_TO_CHECK_CONSOLE_FILE:
        if value not in output:
            missing.append(value)

    assert not missing, f"Missing values in console output: {missing}"
    LOGGER.info("Console logging test passed")


def test_mw_log_file_logging(target, datarouter_on_target):
    """Verify all supported data types appear in DLT file logging output."""
    target.execute("cd /opt/test_apps/logging_app && ./bin/logging_app")

    local_dir = tempfile.mkdtemp(prefix="dlt_file_")
    local_path = os.path.join(local_dir, "EXA.dlt")
    target.download("/tmp/EXA.dlt", local_path)
    LOGGER.info(f"Downloaded DLT file: {os.path.getsize(local_path)} bytes")

    messages = python_dlt.load(local_path)
    payloads = []
    for msg in messages:
        try:
            payload = msg.payload_decoded
            if isinstance(payload, bytes):
                payload = payload.decode("utf-8", errors="replace")
            payloads.append(str(payload))
        except Exception:
            pass

    combined_output = " ".join(payloads)
    LOGGER.info(f"File logging combined output length: {len(combined_output)} chars")

    missing = []
    for value in VALUES_TO_CHECK_CONSOLE_FILE:
        if value not in combined_output:
            missing.append(value)

    assert not missing, f"Missing values in file DLT output: {missing}"
    LOGGER.info("File logging test passed")
