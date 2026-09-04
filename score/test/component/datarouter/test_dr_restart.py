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

"""Migration status for SPP's datarouter auto-restart-on-crash test.

SPP's version (test_logging_dr_restart.py::test_datarouter_restart) does:
    execute_command(ssh, "killall -9 datarouter")
    time.sleep(1)
    assert execute_command(ssh, "pidof datarouter", timeout=60) == 0

That assertion only holds because SPP's target runs datarouter as a systemd
service with a Restart=always policy -- systemd, not the test, is what
relaunches the process after the kill. The test itself contains no restart
logic at all.

SCORE's test infrastructure has no equivalent service manager or process
supervisor. Evidence checked directly in this repo before concluding that:
  - quality/integration_testing/environments/qnx8_qemu/system.build has no
    service-manager entry for datarouter, only a direct IFS binary mapping
    ("[perms=777] /usr/bin/datarouter/datarouter = ${DATAROUTER}").
  - score/test/component/logging_plugin.py's datarouter_on_target fixture
    launches datarouter with a single direct execute_async/exec call
    (Docker) or a single "on ... ./datarouter" invocation (QNX) -- in both
    cases a one-shot launch with no restart wrapper.
  - No systemd unit, supervisor process, or Restart=/respawn configuration
    exists anywhere under score/test/component/ or
    quality/integration_testing/ (grepped for "systemd", "supervisor",
    "Restart=", "respawn" -- zero matches).

This is infrastructure the SUT's test deployment doesn't have, not a test
authoring gap -- per the ticket, migrating means porting the test against
what SCORE already provides, not building new supervisory infrastructure to
make an unsupported scenario pass. Skipped, not deleted, so the gap stays
visible in test output/reports.
"""

import pytest


@pytest.mark.skip(
    reason=(
        "SPP's test relies on systemd's Restart=always policy to relaunch "
        "datarouter after a kill -- SCORE's Docker/QNX test images run no "
        "service manager or process supervisor at all (verified: no "
        "systemd/supervisor/Restart=/respawn configuration anywhere in "
        "quality/integration_testing/ or score/test/component/). Not "
        "portable without adding new supervisory infrastructure, which is "
        "out of scope for this migration."
    )
)
def test_dr_restart():
    pass
