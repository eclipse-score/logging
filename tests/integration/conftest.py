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

import logging
import time
import pytest

from itf.plugins.com.ssh import execute_command, execute_command_output

logger = logging.getLogger(__name__)


def pytest_addoption(parser):
    parser.addoption(
        "--target-ip",
        action="store",
        required=True,
        help="Target IP address for SSH connection to target (QEMU VM)",
    )


@pytest.hookimpl(trylast=True)
def pytest_sessionstart(session):
    """Override test target config IP with the current target (QEMU VM)

    We use the S_CORE_ECU_QEMU_BRIDGE_NETWORK_PP target config
    for the tests, but the actual IP address of the target (QEMU VM) is
    assigned dynamically via DHCP. This hook updates the target
    configuration for the test session.
    """
    from itf.plugins.base.base_plugin import TARGET_CONFIG_KEY

    if TARGET_CONFIG_KEY in session.stash:
        target_ip_address = session.config.getoption("--target-ip")
        target_config = session.stash[TARGET_CONFIG_KEY]
        target_config._BaseProcessor__ip_address = target_ip_address
        logger.info("Connecting to target IP: %s", target_ip_address)


_DATAROUTER_CHECK_CMD = "/proc/boot/pidin | /proc/boot/grep datarouter"
# pathspace ability provides the datarouter access to the `procnto` pathname prefix space
# required for mw/com message passing with mw::log frontend
_DATAROUTER_START_CMD = (
    "cd /usr/bin && on -A nonroot,allow,pathspace -u 1051:1091 "
    "./datarouter --no_adaptive_runtime &"
)
_DATAROUTER_STARTUP_TIMEOUT_SEC = 2


@pytest.fixture(scope="module")
def datarouter_running(target_fixture):
    with target_fixture.sut.ssh() as ssh:
        exit_status, stdout_lines, _ = execute_command_output(ssh, _DATAROUTER_CHECK_CMD)
        output = "\n".join(stdout_lines)

        if "datarouter" not in output:
            logger.info("Datarouter not running. Starting Datarouter..")
            execute_command(ssh, _DATAROUTER_START_CMD)
            time.sleep(_DATAROUTER_STARTUP_TIMEOUT_SEC)

            _, stdout_lines, _ = execute_command_output(ssh, _DATAROUTER_CHECK_CMD)
            if "datarouter" not in "\n".join(stdout_lines):
                pytest.fail("Failed to start datarouter on target")
            logger.info("Datarouter started successfully..")
        else:
            logger.info("Datarouter already running!")
    yield


@pytest.fixture(scope="module")
def dlt_receiver_config(target_config_fixture):
    data_router_config = target_config_fixture.data_router_config
    return {
        "vlan_address": data_router_config["vlan_address"],
        "multicast_addresses": data_router_config["multicast_addresses"],
    }
