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

"""Integration test for mw::log verbose log-level filtering (instance Logger API).

Same verification as test_mw_verbose_filters but exercises the AUTOSAR-style
CreateLogger() instance API instead of free functions.
"""

import logging

from dlt_parser import count_messages_by_context


LOGGER = logging.getLogger(__name__)

TOTAL_VERBOSE_MESSAGES = 22
TEST_APP_ID = "TAP"
CONTEXT_IDS = {"FAT", "ERR", "WRN", "INF", "DBG", "VBS"}


def test_ara_verbose_filters(target, datarouter_on_target, dlt_capture):
    """Verify per-context verbose log-level filtering with instance Logger API.

    Same expected counts as test_mw_verbose_filters:
    FAT=1, ERR=2, WRN=3, INF=4, DBG=5, VBS=6, total=22
    """
    with dlt_capture() as receiver:
        target.execute("cd /opt/test_apps/arafiltertest && ./bin/arafiltertest")

    output = receiver.get_output()
    LOGGER.info(f"DLT output length: {len(output)} chars")

    counts, total = count_messages_by_context(output, TEST_APP_ID, CONTEXT_IDS)

    assert counts.get("FAT", 0) == 1, f"FAT: expected 1, got {counts.get('FAT', 0)}"
    assert counts.get("ERR", 0) == 2, f"ERR: expected 2, got {counts.get('ERR', 0)}"
    assert counts.get("WRN", 0) == 3, f"WRN: expected 3, got {counts.get('WRN', 0)}"
    assert counts.get("INF", 0) == 4, f"INF: expected 4, got {counts.get('INF', 0)}"
    assert counts.get("DBG", 0) == 5, f"DBG: expected 5, got {counts.get('DBG', 0)}"
    assert counts.get("VBS", 0) == 6, f"VBS: expected 6, got {counts.get('VBS', 0)}"
    assert total == TOTAL_VERBOSE_MESSAGES, (
        f"Total: expected {TOTAL_VERBOSE_MESSAGES}, got {total}"
    )
