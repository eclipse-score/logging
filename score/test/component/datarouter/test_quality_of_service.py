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

import collections
import logging

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

APP_ID = "LGGG"

MAX_TOLERATED_MISSING_MESSAGES_PERCENT = 0.5
_GENERATION_ITERATIONS = 1500
_GENERATION_SLEEP_MS = 10


def _missing_messages(counters):
    missing = 0
    for previous, current in zip(counters, counters[1:]):
        if (previous + 1) % 256 != current:
            missing += ((current - previous) % 256) - 1
    return missing


def test_missing_messages(target, datarouter_on_target, dlt_capture):
    """Verify the DLT message loss on each channel stays below the tolerated threshold."""
    with dlt_capture() as receiver:
        target.execute(
            f"cd /opt/test_apps/dlt_generator && "
            f"./bin/dlt_generator -i {_GENERATION_ITERATIONS} -s {_GENERATION_SLEEP_MS}"
        )

    record = download_dlt(target, receiver.dlt_file)
    messages = record.find(include_non_ext=True)
    assert any(m.apid == APP_ID for m in messages), f"No messages from {APP_ID}"

    counters = collections.defaultdict(list)
    for m in messages:
        counters[m.raw_msg.ecuid].append(m.raw_msg.mcnt)

    for ecu, values in sorted(counters.items()):
        missing = _missing_messages(values)
        percent = 100.0 * missing / (missing + len(values))
        LOGGER.info(
            f"{ecu}: received {len(values)}, missing {missing} ({percent:.2f} %)"
        )
        assert percent < MAX_TOLERATED_MISSING_MESSAGES_PERCENT, (
            f"{ecu}: lost {percent:.2f} % of DLT messages"
        )
