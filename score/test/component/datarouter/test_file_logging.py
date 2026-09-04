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

"""Integration test for DLT kFile logging via dlt_generator.

Verifies that dlt_generator writes the expected number of DLT messages to a
local .dlt file when configured with kFile logging mode. Ported from SPP's
test_file_logging.py::test_dlt_kfile_logging.
"""

import logging
import os
import tempfile

import dlt.dlt as python_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"
ITERATIONS = 12
DEFAULT_MESSAGE = "default message text for example log generating application"
# use_full_output defaults to true in dlt_generator: Fatal + Error + Warn + Info + Verbose + Debug
MESSAGES_PER_ITERATION = 6


def test_dlt_kfile_logging(target, datarouter_on_target):
    """Verify DLT kFile logging produces the expected number of messages."""
    expected = ITERATIONS * MESSAGES_PER_ITERATION

    target.execute(
        f"cd /opt/test_apps/dlt_generator && ./bin/dlt_generator -i {ITERATIONS} -s 0"
    )

    exit_code, _ = target.execute("test -f /tmp/LGGG.dlt && echo EXISTS")
    assert exit_code == 0, "DLT file /tmp/LGGG.dlt was not created on the target"

    with tempfile.TemporaryDirectory() as tmpdir:
        local_dlt = os.path.join(tmpdir, "LGGG.dlt")
        target.download("/tmp/LGGG.dlt", local_dlt)
        LOGGER.info(f"Downloaded DLT file: {os.path.getsize(local_dlt)} bytes")

        dlt_messages = python_dlt.load(local_dlt, None)
        occurrences = sum(
            1
            for m in dlt_messages
            if getattr(m, "apid", None) == APP_ID
            and DEFAULT_MESSAGE in str(getattr(m, "payload_decoded", ""))
        )
        LOGGER.info(f"Expected {expected} occurrences, got {occurrences}")
        assert occurrences == expected
