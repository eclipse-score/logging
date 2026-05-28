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

"""Structured parser for ``dlt-receive -a`` text output.
DLT text lines produced by ``dlt-receive -a`` have the form::
    <timestamp> <ecuid> <apid> <ctid> <type> <subtype> <mode> <noar> <payload ...>
Field positions can vary (some fields may be absent in non-extended
headers), but the *apid* and *ctid* fields always appear as a
whitespace-separated pair.  DLT pads short IDs to 4 characters with
``-`` (e.g. ``TAP`` → ``TAP-``).
This module provides helpers that work on the *text* output captured by
``dlt-receive -a --stdout-flush`` (which is what ``dlt_on_target``
returns via ``receiver.get_output()``).
"""

from collections import defaultdict
from dataclasses import dataclass, field


@dataclass
class DltMessage:
    """A single parsed DLT message."""

    apid: str
    ctid: str
    payload: str
    raw_line: str


def parse_messages(dlt_output: str, app_id: str) -> list[DltMessage]:
    """Parse DLT text output and return messages matching *app_id*.
    Args:
        dlt_output: Full text output from ``dlt-receive -a``.
        app_id: Application ID to filter on (without padding).
    Returns:
        List of :class:`DltMessage` instances whose *apid* matches *app_id*.
    """
    messages: list[DltMessage] = []
    for line in dlt_output.splitlines():
        parts = line.split()
        for i, part in enumerate(parts):
            if part.rstrip("-") == app_id and i + 1 < len(parts):
                ctid = parts[i + 1].rstrip("-")
                # Everything after apid and ctid is the payload
                payload = " ".join(parts[i + 2 :]) if i + 2 < len(parts) else ""
                messages.append(
                    DltMessage(
                        apid=app_id,
                        ctid=ctid,
                        payload=payload,
                        raw_line=line,
                    )
                )
                break
    return messages


def count_messages_by_context(
    dlt_output: str,
    app_id: str,
    context_ids: set[str] | None = None,
) -> tuple[dict[str, int], int]:
    """Count messages per context ID for a given application.
    Args:
        dlt_output: Full text output from ``dlt-receive -a``.
        app_id: Application ID to filter on.
        context_ids: Optional set of context IDs to include in the
            per-context breakdown.  Messages with other context IDs are
            still included in the total count.
    Returns:
        A ``(counts, total)`` tuple where *counts* maps context ID to
        message count and *total* is the overall number of messages from
        *app_id*.
    """
    counts: dict[str, int] = defaultdict(int)
    total = 0
    for msg in parse_messages(dlt_output, app_id):
        total += 1
        if context_ids is None or msg.ctid in context_ids:
            counts[msg.ctid] += 1
    return dict(counts), total
