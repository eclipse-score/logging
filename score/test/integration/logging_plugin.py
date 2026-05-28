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

"""Shared pytest plugin for score_logging integration tests.
Provides Docker container configuration, a datarouter lifecycle fixture,
and DLT utility functions used by all logging ITF tests.
"""

import logging
import os
import tempfile
import time
from contextlib import contextmanager

import pytest
from score.itf.plugins.dlt.dlt_receive import Protocol
from score.itf.plugins.dlt.dlt_window import DltLogRecord

LOGGER = logging.getLogger(__name__)

# UDP multicast group addresses for DLT — dlt-receive joins these groups
# to capture messages sent by the datarouter.
DLT_MULTICAST_IPS = ["239.255.42.99"]

# Settle time (seconds) after dlt-receive starts, to allow the multicast group
# join to propagate before the application under test begins sending.
_DLT_RECEIVER_SETTLE_DELAY = 0.2

# Datarouter readiness polling parameters.
_DATAROUTER_READY_TIMEOUT = 5.0
_DATAROUTER_READY_INTERVAL = 0.1


def _wait_for_datarouter(target, timeout=_DATAROUTER_READY_TIMEOUT):
    """Poll /proc/net/unix for the datarouter abstract socket to appear."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        exit_code, _ = target.execute("grep -q datarouter /proc/net/unix")
        if exit_code == 0:
            LOGGER.info("Datarouter socket detected")
            return
        time.sleep(_DATAROUTER_READY_INTERVAL)
    raise TimeoutError(f"Datarouter socket not found within {timeout}s")


def download_dlt(target, remote_path):
    """Download a .dlt file from the target and return a DltLogRecord for querying.
    The file is stored in a temporary directory that persists for the process
    lifetime.  Use ``record.find()`` to query verbose/non-verbose messages.
    """
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
    """Start the datarouter (DLT daemon) on the Docker target.
    Launches the datarouter in non-adaptive mode as a background process,
    polls for socket readiness, and stops it on teardown.
    """
    proc = target.execute_async(
        "/opt/datarouter/bin/datarouter",
        args=["--no_adaptive_runtime"],
        cwd="/opt/datarouter",
    )
    _wait_for_datarouter(target)
    yield target
    if proc.is_running():
        proc.stop()


@pytest.fixture(scope="function")
def dlt_capture(target, dlt_on_target):
    """Start a DLT receiver with deterministic multicast reception.
    Wraps the framework's ``dlt_on_target`` with default multicast parameters
    and a settle delay to ensure the multicast group join propagates before the
    application under test begins sending.
    Usage in tests::
        with dlt_capture() as receiver:
            target.execute(app_cmd)
        output = receiver.get_output()
    """

    @contextmanager
    def _start(protocol=Protocol.UDP, host_ip=None, multicast_ips=None):
        if host_ip is None:
            host_ip = target.get_ip()
        if multicast_ips is None:
            multicast_ips = DLT_MULTICAST_IPS
        with dlt_on_target(
            protocol, host_ip=host_ip, multicast_ips=multicast_ips
        ) as receiver:
            time.sleep(_DLT_RECEIVER_SETTLE_DELAY)
            yield receiver

    return _start
