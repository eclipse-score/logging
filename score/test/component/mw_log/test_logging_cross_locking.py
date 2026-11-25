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

import logging
import os
import re
import time

from attribute_plugin import add_test_properties

LOGGER = logging.getLogger(__name__)

_APP_CMD = "cd /opt/test_apps/cross_locking_app && nohup ./bin/cross_locking_app > /dev/null 2>&1 &"
_TRACE_BUFFER = "/dev/shmem/tracebuffer.kev"
_TRACE_TEXT = "/tmp/traceprinter.txt"
_APP_START_WAIT_SEC = 1.5
_VIOLATION_STATES = [
    "THCONDVAR",
    "THNET_REPLY",
    "THNET_SEND",
    "THMUTEX",
    "THRECEIVE",
    "THREPLY",
    "THSEM",
    "THSEND",
]


def _app_pids(target, name):
    _, out = target.execute(f"pidin -p {name} -F '%a'")
    return [
        int(token) for token in out.decode(errors="replace").split() if token.isdigit()
    ]


def _logger_thread_ids(target, app_pid):
    _, out = target.execute(f"pidin -p {app_pid} threads")
    tids = set()
    for line in out.decode(errors="replace").splitlines():
        if "LoggerThread" in line:
            tids.add(int(line.split()[1]))
    return tids


def _violations(lines, app_pid, logger_tids):
    found = []
    for line in lines:
        if str(app_pid) not in line or "tid" not in line:
            continue
        match = re.search(r"tid:(\d+)", line)
        if not match or int(match.group(1)) not in logger_tids:
            continue
        found.extend(
            f"{state}: {line.strip()}" for state in _VIOLATION_STATES if state in line
        )
    return found


@add_test_properties(
    fully_verifies=["comp_req__log__cross_locking"],
    test_type="requirements-based",
    derivation_technique="design-analysis",
)
def test_logging_cross_locking(target, datarouter_on_target):
    """Verify a thread logging in a loop does not block other logging threads."""
    exit_code, _ = target.execute(_APP_CMD)
    assert exit_code == 0, "Failed to start cross_locking_app"
    time.sleep(_APP_START_WAIT_SEC)
    app_pids = _app_pids(target, "cross_locking_app")
    assert app_pids, "cross_locking_app is not running"
    app_pid = app_pids[0]

    try:
        logger_tids = _logger_thread_ids(target, app_pid)
        assert logger_tids, f"Could not find any LoggerThread TIDs for PID {app_pid}"
        LOGGER.info("Logger thread TIDs: %s", sorted(logger_tids))

        target.execute(f"rm -f {_TRACE_BUFFER} {_TRACE_TEXT}")
        exit_code, _ = target.execute(
            f"on -p 45 /usr/sbin/tracelogger -w -s 10 -M -S30M -f {_TRACE_BUFFER}"
        )
        assert exit_code == 0, "tracelogger failed"
    finally:
        target.execute(f"kill -s TERM {app_pid} 2>/dev/null; true")

    exit_code, _ = target.execute(
        f"/usr/bin/traceprinter -n -f {_TRACE_BUFFER} | grep -F {app_pid} > {_TRACE_TEXT}"
    )
    local_dir = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR", "/tmp")
    local_path = os.path.join(local_dir, os.path.basename(_TRACE_TEXT))
    target.download(_TRACE_TEXT, local_path)
    target.execute(f"rm -f {_TRACE_BUFFER} {_TRACE_TEXT}")

    with open(local_path, encoding="utf-8", errors="replace") as trace:
        lines = trace.readlines()
    assert lines, "traceprinter produced no events for cross_locking_app"

    violations = _violations(lines, app_pid, logger_tids)
    assert not violations, (
        f"Cross-locking violations in logger threads: {violations[:10]}"
    )
