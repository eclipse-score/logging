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

"""Integration test for mw::log with all data types.

Verifies that the logging_app binary correctly sends DLT messages containing
all supported data types (bool, integers, hex, bin, raw buffer, slog2, etc.)
via the datarouter, and that they are received by dlt-receive with correct
content. Also verifies console logging output and file (.dlt) logging.
"""

import logging
import os
import tempfile

import dlt.dlt as python_dlt
from logging_plugin import download_dlt


LOGGER = logging.getLogger(__name__)

LOGGING_APP_ID = "EXA"
LOGGING_APP_CMD = "cd /opt/test_apps/logging_app && ./bin/logging_app"

# Expected substrings for DLT binary file output parsed by python-dlt.
# Differs from dlt-receive -a (remote) in:
#   - UINT64_MAX renders as 18446744073709551615 (unsigned, not -1)
#   - log_raw_buffer renders as 72'61'77 (apostrophe-separated hex)
VALUES_TO_CHECK_FILE = [
    "Logging Application DoLogging",
    "val_bool 1",
    "val_uint8t 123 val_uint16t 1234 val_uint32t 12345 val_uint64t 123456",
    "val_uint8tmax 255 val_uint16tmax 65535 val_uint32tmax 4294967295 val_uint64tmax 18446744073709551615",
    "val_int8t -34 val_int16t -14576 val_int32t -2147483640 val_int64t -9223372036854775700",
    "val_int8tmax 127 val_int16tmax 32767 val_int32tmax 2147483647 val_int64tmax 9223372036854775807",
    "val_int8tmin -128 val_int16tmin -32768 val_int32tmin -2147483648 val_int64tmin -9223372036854775808",
    "val_int8tminplusint8t -94 val_int16tminplusint16t -18192 val_int32tminplusint32t -8 val_int64tminplusint64t -108",
    "val_string Logging",
    "val_double 93454.6",
    "log_hex_8 10 log_hex_16 9876 log_hex_32 543210987 log_hex_64 654321098765432109",
    "log_bin_8 8 log_bin_16 9012 log_bin_32 3456789012 log_bin_64 3456789012345678901",
    "log_raw_buffer",
    "log_slog2_message slog2_message",
]

# Expected substrings for console output (captured from the app's stdout).
VALUES_TO_CHECK_CONSOLE = [
    "Logging Application DoLogging",
    "val_bool True",
    "val_uint8t 123 val_uint16t 1234 val_uint32t 12345 val_uint64t 123456",
    "val_uint8tmax 255 val_uint16tmax 65535 val_uint32tmax 4294967295 val_uint64tmax 18446744073709551615",
    "val_int8t -34 val_int16t -14576 val_int32t -2147483640 val_int64t -9223372036854775700",
    "val_int8tmax 127 val_int16tmax 32767 val_int32tmax 2147483647 val_int64tmax 9223372036854775807",
    "val_int8tmin -128 val_int16tmin -32768 val_int32tmin -2147483648 val_int64tmin -9223372036854775808",
    "val_int8tminplusint8t -94 val_int16tminplusint16t -18192 val_int32tminplusint32t -8 val_int64tminplusint64t -108",
    "val_string Logging",
    "val_double 93454.600000",
    "log_hex_8 10 log_hex_16 9876 log_hex_32 543210987 log_hex_64 654321098765432109",
    "log_bin_8 8 log_bin_16 9012 log_bin_32 3456789012 log_bin_64 3456789012345678901",
    "log_raw_buffer 726177",
    "log_slog2_message slog2_message",
]


def test_mw_log_remote_logging(target, datarouter_on_target, dlt_capture):
    """Verify all data types are correctly logged and received via DLT."""
    with dlt_capture() as receiver:
        target.execute(LOGGING_APP_CMD)

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(query=dict(apid=LOGGING_APP_ID))
    LOGGER.info(f"Received {len(messages)} messages from {LOGGING_APP_ID}")
    for msg in messages:
        LOGGER.info(f"  [{msg.ctid}] {msg.payload}")

    assert len(messages) > 0, "No DLT messages received from logging_app"

    app_output = "\n".join(str(msg.payload) for msg in messages)
    missing = [v for v in VALUES_TO_CHECK_FILE if v not in app_output]
    assert not missing, f"Missing expected values in DLT output: {missing}"


def test_mw_log_console_logging(target, datarouter_on_target):
    """Verify all data types are correctly logged to console (stdout)."""
    proc = target.execute_async(
        "/opt/test_apps/logging_app/bin/logging_app",
        cwd="/opt/test_apps/logging_app",
    )
    proc.wait(timeout_s=10)
    console_output = proc.get_output()
    LOGGER.info(f"Console output length: {len(console_output)} chars")
    LOGGER.info(f"Console output:\n{console_output}")

    assert len(console_output) > 0, "No console output captured from logging_app"

    missing = [v for v in VALUES_TO_CHECK_CONSOLE if v not in console_output]
    assert not missing, f"Missing expected values in console output: {missing}"


def test_mw_log_file_logging(target, datarouter_on_target):
    """Verify all data types are correctly logged to a .dlt file on disk."""
    target.execute(LOGGING_APP_CMD)

    with tempfile.TemporaryDirectory() as tmpdir:
        local_dlt = os.path.join(tmpdir, "EXA.dlt")
        target.download("/tmp/EXA.dlt", local_dlt)
        LOGGER.info(f"Downloaded DLT file: {os.path.getsize(local_dlt)} bytes")

        messages = python_dlt.load(local_dlt, None)
        payloads = []
        for msg in messages:
            if getattr(msg, "apid", None) == LOGGING_APP_ID:
                payload = msg.payload_decoded
                if isinstance(payload, bytes):
                    payload = payload.decode(errors="ignore")
                payloads.append(str(payload))

        LOGGER.info(f"Parsed {len(payloads)} messages from DLT file")
        for p in payloads:
            LOGGER.info(f"  FILE payload: {p}")
        assert len(payloads) > 0, "No messages from logging_app in DLT file"

        merged = "\n".join(payloads)
        missing = [v for v in VALUES_TO_CHECK_FILE if v not in merged]
        assert not missing, f"Missing expected values in DLT file: {missing}"
