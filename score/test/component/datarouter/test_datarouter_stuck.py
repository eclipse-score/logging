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
import time

LOGGER = logging.getLogger(__name__)

_CLIENT_CMD = "cd /opt/test_apps/dlt_generator && nohup ./bin/dlt_generator -i 1000000 > /dev/null 2>&1 &"
_CONNECT_WAIT_SEC = 1.0
_ACQUIRE_WAIT_SEC = 1.5
_STUCK_THREAD_STATES = [
    "SEND",
    "REPLY",
    "THSEND",
    "THNET_SEND",
    "THREPLY",
    "THNET_REPLY",
]


def _app_pids(target, name):
    _, out = target.execute(f"pidin -p {name} -F '%a'")
    return [
        int(token) for token in out.decode(errors="replace").split() if token.isdigit()
    ]


def _datarouter_is_alive(target):
    return bool(_app_pids(target, "datarouter"))


def _datarouter_threads(target):
    _, out = target.execute("pidin -p datarouter threads")
    lines = [
        line
        for line in out.decode(errors="replace").splitlines()
        if "datarouter" in line
    ]
    LOGGER.info("datarouter threads:\n%s", "\n".join(lines))
    return lines


def test_datarouter_stuck_on_client_sigstop(target, datarouter_on_target):
    """Verify a frozen logging client does not block datarouter threads."""
    exit_code, _ = target.execute(_CLIENT_CMD)
    assert exit_code == 0, "Failed to start dlt_generator"
    time.sleep(_CONNECT_WAIT_SEC)
    client_pids = _app_pids(target, "dlt_generator")
    assert client_pids, "dlt_generator is not running"
    client_pid = client_pids[0]
    LOGGER.info("Client PID: %d", client_pid)

    try:
        exit_code, _ = target.execute(f"kill -s STOP {client_pid}")
        assert exit_code == 0, f"Failed to stop client {client_pid}"
        _, out = target.execute(f"pidin -p {client_pid} -F '%J'")
        assert "STOPPED" in out.decode(errors="replace"), (
            f"Client {client_pid} is not stopped"
        )
        time.sleep(_ACQUIRE_WAIT_SEC)

        assert _datarouter_is_alive(target), (
            "datarouter died while the client was frozen"
        )
        threads = _datarouter_threads(target)
        assert threads, "pidin listed no datarouter threads"
        stuck = [
            line
            for line in threads
            if str(client_pid) in line
            and any(state in line for state in _STUCK_THREAD_STATES)
        ]
        assert not stuck, (
            f"datarouter threads blocked on frozen client {client_pid}: {stuck}"
        )
    finally:
        target.execute(
            f"kill -s CONT {client_pid} 2>/dev/null; kill -s TERM {client_pid} 2>/dev/null; true"
        )
