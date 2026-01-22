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
import os
import time

from itf.plugins.dlt.dlt_receive import DltReceive, Protocol


logger = logging.getLogger(__name__)

# Datarouter messages with Context ID "STAT" sent every ~5s hence 10s to reliably capture and verify
CAPTURE_DURATION_SECONDS = 10

# DLT message identifiers for datarouter statistics
APP_ID = "DR"
CTX_ID = "STAT"


def test_dlt_capture(datarouter_running, test_config_fixture, dlt_receiver_config):
    """Verify DLT messages can be captured from the datarouter.
    """
    dlt_file = "/tmp/test_dlt_capture.dlt"

    vlan_address = dlt_receiver_config["vlan_address"]
    multicast_addresses = dlt_receiver_config["multicast_addresses"]

    # TODO: Replace with DltWindow when fixed in ITF.
    with DltReceive(
        target_ip=vlan_address,
        protocol=Protocol.UDP,
        file_name=dlt_file,
        binary_path=test_config_fixture.dlt_receive_path,
        data_router_config={
            "vlan_address": vlan_address,
            "multicast_addresses": multicast_addresses,
        },
    ):
        time.sleep(CAPTURE_DURATION_SECONDS)

    assert os.path.exists(dlt_file), f"DLT file not created: {dlt_file}"

    with open(dlt_file, "rb") as f:
        dlt_data = f.read()

    logger.info("DLT file size: %d bytes", len(dlt_data))

    # DLT extended header: APP-ID (4 bytes) followed by CTX-ID (4 bytes)
    pattern = f"{APP_ID}\x00\x00{CTX_ID}".encode()
    message_count = dlt_data.count(pattern)

    logger.info("Found %d messages with app_id=%s, context_id=%s", message_count, APP_ID, CTX_ID)

    assert message_count > 1, f"Expected more than 1 message, but got {message_count}"
