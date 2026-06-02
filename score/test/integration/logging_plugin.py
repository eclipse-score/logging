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

"""Shared pytest plugin for score_logging integration tests."""

import logging
import os
import tempfile
import time
from contextlib import contextmanager

import pytest
from score.itf.plugins.dlt.dlt_receive import Protocol
from score.itf.plugins.dlt.dlt_window import DltLogRecord

LOGGER = logging.getLogger(__name__)

DLT_MULTICAST_IPS = ["239.255.42.99"]
_DLT_RECEIVER_SETTLE_DELAY = 0.2
_DATAROUTER_READY_TIMEOUT = 10.0
_DATAROUTER_READY_INTERVAL = 0.2

_QNX_DR_CMD = (
    "on -A nonroot,allow,pathspace -u 1000:1000 "
    "/usr/bin/datarouter/datarouter --no_adaptive_runtime "
    "> /dev/null 2>&1 &"
)
_QNX_DR_STOP_CMD = (
    "slay datarouter 2>/dev/null; sleep 1; slay -s SIGKILL datarouter 2>/dev/null; true"
)

_qnx_cache: dict = {}


def _is_qnx(target) -> bool:
    target_id = id(target)
    if target_id not in _qnx_cache:
        exit_code, out = target.execute("uname -s")
        if exit_code != 0:
            raise RuntimeError(
                f"Platform detection failed: uname -s returned exit code {exit_code}"
            )
        result = (b"QNX" in out) if isinstance(out, bytes) else ("QNX" in out)
        _qnx_cache[target_id] = result
    return _qnx_cache[target_id]


def _wait_for_datarouter(target, timeout=_DATAROUTER_READY_TIMEOUT):
    deadline = time.monotonic() + timeout
    is_qnx = _is_qnx(target)
    while time.monotonic() < deadline:
        if is_qnx:
            exit_code, out = target.execute("/proc/boot/pidin arg")
            if exit_code == 0:
                output = out.decode() if isinstance(out, bytes) else out
                if "datarouter" in output:
                    LOGGER.info("Datarouter ready")
                    return
        else:
            exit_code, _ = target.execute("grep -q datarouter /proc/net/unix")
            if exit_code == 0:
                LOGGER.info("Datarouter ready")
                return
        time.sleep(_DATAROUTER_READY_INTERVAL)
    raise TimeoutError(f"Datarouter not ready within {timeout}s")


def download_dlt(target, remote_path):
    """Download a .dlt file from the target and return a DltLogRecord."""
    local_dir = tempfile.mkdtemp(prefix="dlt_")
    local_path = os.path.join(local_dir, os.path.basename(remote_path))
    target.download(remote_path, local_path)
    LOGGER.info(f"Downloaded {remote_path}: {os.path.getsize(local_path)} bytes")
    return DltLogRecord(local_path)


@pytest.fixture(scope="session")
def docker_configuration():
    """Configure Docker container for DLT integration tests."""
    return {
        "init": True,
        "shm_size": "2G",
    }


@pytest.fixture(scope="function")
def datarouter_on_target(target):
    """Start the datarouter on the target (Docker or QNX), yield, then stop it."""
    if _is_qnx(target):
        target.execute(_QNX_DR_CMD)
        try:
            _wait_for_datarouter(target)
            yield target
        finally:
            target.execute(_QNX_DR_STOP_CMD)
    else:
        proc = target.execute_async(
            "/opt/datarouter/bin/datarouter",
            args=["--no_adaptive_runtime"],
            cwd="/opt/datarouter",
        )
        try:
            _wait_for_datarouter(target)
            yield target
        finally:
            if proc.is_running():
                proc.stop()


@pytest.fixture(scope="function")
def dlt_capture(target, dlt_on_target, request):
    """Start a DLT receiver. On QNX, binds to the host tap0 IP from dlt_config."""

    @contextmanager
    def _start(protocol=Protocol.UDP, host_ip=None, multicast_ips=None):
        if host_ip is None:
            if _is_qnx(target):
                try:
                    dlt_cfg = request.getfixturevalue("dlt_config")
                    host_ip = dlt_cfg.host_ip
                except pytest.FixtureLookupError:
                    host_ip = target.get_ip()
            else:
                host_ip = target.get_ip()
        if multicast_ips is None:
            multicast_ips = DLT_MULTICAST_IPS
        with dlt_on_target(
            protocol, host_ip=host_ip, multicast_ips=multicast_ips
        ) as receiver:
            time.sleep(_DLT_RECEIVER_SETTLE_DELAY)
            yield receiver

    return _start
