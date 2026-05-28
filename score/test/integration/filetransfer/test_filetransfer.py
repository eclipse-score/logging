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

"""Integration test for DLT file transfer functionality.
Verifies that the score_logging DLT file transfer mechanism correctly
transfers files from the target container via DLT (in-band file transfer
protocol).  The filetransfer_app creates 50 files and transfers each.
"""

import logging

import pytest
from dlt_parser import count_messages_by_context

LOGGER = logging.getLogger(__name__)

APP_ID = "FTEA"
NUM_TRANSFERS = 50


def test_filetransfer(target, datarouter_on_target, dlt_capture):
    """Verify DLT file transfer: lifecycle messages and per-file transfer logs.
    Expected observations:
    - "initialize" log message appears in DLT output
    - "DoFileTransfer" log message appears in DLT output
    - "done" log message appears in DLT output
    - Exactly NUM_TRANSFERS "Transferred file" messages appear
    - Exactly NUM_TRANSFERS files are present under /tmp/ on the target
    """
    with dlt_capture() as receiver:
        target.execute("cd /opt/test_apps/filetransfer && ./bin/filetransfer_app")

    output = receiver.get_output()
    LOGGER.info(f"DLT output length: {len(output)} chars")

    assert "initialize" in output, "Did not find 'initialize' in DLT output"
    assert "DoFileTransfer" in output, "Did not find 'DoFileTransfer' in DLT output"
    assert "done" in output, "Did not find 'done' in DLT output"

    transferred_count = output.count("Transferred file")
    assert transferred_count == NUM_TRANSFERS, (
        f"Expected {NUM_TRANSFERS} 'Transferred file' messages, got {transferred_count}"
    )

    exit_code, ls_output = target.execute(
        f"ls /tmp/filetransfer_test_*.txt 2>/dev/null | wc -l"
    )
    file_count = int(ls_output.strip())
    assert file_count == NUM_TRANSFERS, (
        f"Expected {NUM_TRANSFERS} transferred files on target, found {file_count}"
    )
    LOGGER.info(f"File transfer test passed: {transferred_count} files transferred")


@pytest.mark.skip(reason="Protocol verification requires additional DLT tooling")
def test_filetransfer_protocol_verification(target, datarouter_on_target, dlt_capture):
    """Verify DLT file transfer protocol messages (FLS/FLDA/FLE chunks).
    NOTE: Skipped — requires python-dlt or raw packet inspection tooling.
    """
    pass
