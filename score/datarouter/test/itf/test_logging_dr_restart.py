"""
Execute the ITF to test the datarouter non-adaptive restart on failure
"""
import time
import pytest
from itf.com.ssh import execute_command


@pytest.mark.supported_os(["linux"])  # TODO port to QNX: TicketOld-77539
@pytest.mark.metadata(description="Datarouter shall be configured to be auto-restarted if it terminates unexpectedly.",
                      verifies=[1573713],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements")
def test_datarouter_restart(target_fixture):
    """
    send kill signal to datarouter non-adaptive application
    test if the process is running , since it has to be restarted by systemd
    """
    with target_fixture.sut.ssh() as ssh:
        execute_command(ssh, f"killall -9 datarouter")
        time.sleep(1)
        assert execute_command(ssh, "pidof datarouter", timeout=60) == 0
