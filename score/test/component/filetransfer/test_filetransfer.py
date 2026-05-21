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

"""Integration test for DLT file transfer.

Verifies that the filetransfer_app correctly creates test files and invokes
the FileTransfer API via the datarouter.  The verbose log messages confirming
each transfer are received via dlt-receive.

Note: The datarouter's file transfer feature flag may be disabled in the
default configuration, in which case the FLST/FLDA/FLFI protocol messages
are not emitted.  This test verifies the app-side transfer invocations and
that the test files are created on the target filesystem.
"""

import logging

import pytest
from logging_plugin import download_dlt


LOGGER = logging.getLogger(__name__)

APP_ID = "FTEA"
NUM_TRANSFERS = 50


def test_filetransfer(target, datarouter_on_target, dlt_capture):
    """Verify that file transfer DLT messages are sent and received."""
    with dlt_capture() as receiver:
        target.execute("cd /opt/test_apps/filetransfer && ./bin/filetransfer_app")

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=APP_ID))
    LOGGER.info(f"App message count: {len(messages)}")

    assert len(messages) > 0, "No DLT messages received from filetransfer_app"

    app_payloads = "\n".join(str(m.payload) for m in messages)

    # Verify lifecycle messages
    assert "initialize" in app_payloads, "Missing 'initialize' message"
    assert "DoFileTransfer" in app_payloads, "Missing 'DoFileTransfer' message"
    assert "done" in app_payloads, "Missing 'done' message"

    # Verify transfer confirmations from the app
    transfer_msgs = [m for m in messages if "Transferred file" in str(m.payload)]
    assert len(transfer_msgs) == NUM_TRANSFERS, (
        f"Expected {NUM_TRANSFERS} transfer confirmations, got {len(transfer_msgs)}"
    )

    # Verify the test files were created on the target filesystem
    exit_code, ls_output = target.execute(
        "ls /tmp/filetransfer_test_[0-9]*.txt | wc -l"
    )
    file_count = int(ls_output.strip())
    assert file_count == NUM_TRANSFERS, (
        f"Expected {NUM_TRANSFERS} test files on target, found {file_count}"
    )


@pytest.mark.skip(
    reason="file_transfer_impl not available in this repo — FLST/FLDA/FLFI protocol messages are not emitted"
)
def test_filetransfer_protocol_verification(target, datarouter_on_target, dlt_capture):
    """Verify DLT file transfer protocol messages (FLST/FLDA/FLFI) and hash integrity."""
