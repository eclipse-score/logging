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

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

TOTAL_VERBOSE_MESSAGES = 22
TEST_APP_ID = "TAP"
CONTEXT_IDS = ["FAT", "ERR", "WRN", "INF", "DBG", "VBS"]


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

    record = download_dlt(target, receiver.dlt_file)

    def count(ctx):
        return len(record.find(query=dict(apid=TEST_APP_ID, ctid=ctx)))

    assert count("FAT") == 1, f"FAT: expected 1, got {count('FAT')}"
    assert count("ERR") == 2, f"ERR: expected 2, got {count('ERR')}"
    assert count("WRN") == 3, f"WRN: expected 3, got {count('WRN')}"
    assert count("INF") == 4, f"INF: expected 4, got {count('INF')}"
    assert count("DBG") == 5, f"DBG: expected 5, got {count('DBG')}"
    assert count("VBS") == 6, f"VBS: expected 6, got {count('VBS')}"

    total = sum(count(ctx) for ctx in CONTEXT_IDS) + count("DFLT")
    assert total == TOTAL_VERBOSE_MESSAGES, (
        f"Total: expected {TOTAL_VERBOSE_MESSAGES}, got {total}"
    )
