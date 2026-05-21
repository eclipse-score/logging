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

"""Integration test for datarouter non-verbose (TRACE) log-level filtering.

Verifies that per-context log-level filtering works for non-verbose (TRACE)
messages routed through the datarouter.  The filtertest binary emits 18
TRACE'd structs (6 per context x 3 contexts: AAAA, BBBB, CCCC) plus one
UndefinedStruct.  Each context is configured at a different log level:

  - AAAA (kVerbose): all 6 messages pass the filter
  - BBBB (kInfo):    4 messages pass (Fatal, Error, Warn, Info)
  - CCCC (kFatal):   1 message passes (Fatal only)

Total expected non-verbose messages: 11

NOTE: This test requires building with the nonverbose DLT flag:
    --//score/datarouter/build_configuration_flags:enable_nonverbose_dlt=True
"""

import ctypes
import logging

from logging_plugin import download_dlt

LOGGER = logging.getLogger(__name__)

TOTAL_NONVERBOSE_MESSAGES = 11


def test_datarouter_filters(target, datarouter_on_target, dlt_capture):
    """Verify per-context non-verbose log-level filtering via TRACE().

    Expected message counts per context group (type_id // 100):
      - group 1 (AAAA, kVerbose): 6 messages
      - group 2 (BBBB, kInfo):    4 messages
      - group 3 (CCCC, kFatal):   1 message
    Total non-verbose messages: 11
    """
    with dlt_capture() as receiver:
        target.execute("cd /opt/test_apps/filtertest && ./bin/filtertest")

    # Download the captured .dlt file
    record = download_dlt(target, receiver.dlt_file)

    # Find only non-extended (non-verbose / TRACE) messages
    messages = record.find(include_ext=False, include_non_ext=True)
    LOGGER.info(f"Non-verbose messages found: {len(messages)}")

    # Extract type IDs from the raw DLT payload
    received_ids = {}
    for msg in messages:
        id_t = ctypes.c_uint32
        message_address = ctypes.addressof(msg.raw_msg.databuffer.contents)
        typeid = id_t.from_address(message_address).value

        ctxid = typeid // 100
        if ctxid not in received_ids:
            received_ids[ctxid] = []
        received_ids[ctxid].append(typeid)

    LOGGER.info(f"Received IDs by context group: {received_ids}")

    assert len(received_ids.get(1, [])) == 6, (
        f"AAAA (kVerbose): expected 6, got {len(received_ids.get(1, []))}"
    )
    assert len(received_ids.get(2, [])) == 4, (
        f"BBBB (kInfo): expected 4, got {len(received_ids.get(2, []))}"
    )
    assert len(received_ids.get(3, [])) == 1, (
        f"CCCC (kFatal): expected 1, got {len(received_ids.get(3, []))}"
    )
    assert len(messages) == TOTAL_NONVERBOSE_MESSAGES, (
        f"Total: expected {TOTAL_NONVERBOSE_MESSAGES}, got {len(messages)}"
    )
    LOGGER.info("Datarouter non-verbose filter validation succeeded")
