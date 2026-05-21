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

"""Integration test for mw::log verbose log-level filtering (free-function API).

Verifies that per-context log-level filtering works correctly when using the
score::mw::log free-function API (LogFatal("CTX"), LogError("CTX"), etc.).

The mwfiltertest binary emits 6 messages (Fatal through Verbose) to each of
6 contexts (FAT, ERR, WRN, INF, DBG, VBS), each configured at a different
log level, plus 1 default-context LogFatal. With the configured filters,
exactly 22 messages should be visible.
"""

import logging

from dlt_parser import count_messages_by_context


LOGGER = logging.getLogger(__name__)

TOTAL_VERBOSE_MESSAGES = 22
TEST_APP_ID = "TAP"
CONTEXT_IDS = {"FAT", "ERR", "WRN", "INF", "DBG", "VBS"}


def test_mw_verbose_filters(target, datarouter_on_target, dlt_capture):
    """Verify per-context verbose log-level filtering with free-function API.

    Expected message counts per context (based on configured log levels):
    - FAT (kFatal):   1 message  (only Fatal passes)
    - ERR (kError):   2 messages (Fatal + Error pass)
    - WRN (kWarn):    3 messages (Fatal + Error + Warn pass)
    - INF (kInfo):    4 messages (Fatal + Error + Warn + Info pass)
    - DBG (kDebug):   5 messages (Fatal + Error + Warn + Info + Debug pass)
    - VBS (kVerbose): 6 messages (all pass)
    Plus 1 default-context LogFatal = 22 total.
    """
    with dlt_capture() as receiver:
        target.execute("cd /opt/test_apps/mwfiltertest && ./bin/mwfiltertest")

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
