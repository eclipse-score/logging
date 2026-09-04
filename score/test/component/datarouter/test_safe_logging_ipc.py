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

"""Migration status for SPP's safe-logging-IPC-disconnect test.

SPP's own test_safe_logging_ipc.py has exactly one test function,
test_datarouter_when_app_disconnect_immediate, and it is itself decorated
with:

    @pytest.mark.skip(reason="Temporary disabled, should be enabled when
    SWP-132779 would be implemented")

i.e. SPP's own team does not run this test either -- it's pending an
internal SPP ticket (SWP-132779) that has not been implemented as of the
pinned commit this migration is based on
(fc1229f5c0c6ddc4eafa29dbaab7b0e82a150055). There is nothing to migrate a
*working* version of; migrating it faithfully means preserving the same
skip, not inventing a fix SPP itself hasn't shipped.
"""

import pytest


@pytest.mark.skip(
    reason=(
        "SPP's own version of this test is itself skipped "
        '("Temporary disabled, should be enabled when SWP-132779 would be '
        'implemented") as of the pinned commit '
        "fc1229f5c0c6ddc4eafa29dbaab7b0e82a150055 -- there is no working "
        "version to migrate. SWP-132779 is an internal SPP ticket outside "
        "this repo's visibility/control."
    )
)
def test_safe_logging_ipc():
    pass
