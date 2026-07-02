import re
import time
import logging
import pytest
import tenacity
from xtf_common.retry import xtf_retry
from xtf_common.process.dlt_window import DltWindow
from itf.com.ssh import execute_command
from itf.com.ssh_command import SshCommand
from itf.actions.actions import is_app_running
from itf.actions.remote_binary_execution import RemoteBinaryExecution

logger = logging.getLogger(__name__)

APP_PATH = "platform/aas/pas/logging/test/itf/"
APP_BIN = "dlt_generator"
SLEEP_BEFORE_SHUTTINGDOWN = 6000

MINIMAL_LOGGING_CONFIG = {
    "appId": "LOGG",
    "appDesc": "DLT generator",
    "logMode": "kRemote",
    "logLevel": "kVerbose",
    "logLevelThresholdConsole": "kVerbose",
    "numberOfSlots": 10000,
    "slotSizeBytes": 1500,
    "ringBufferSize": 100097152
}


class DataRouterManager:
    def __init__(self, target_fixture):
        self.target_fixture = target_fixture

    def __enter__(self):
        return self

    def __exit__(self, exc_type, value, traceback):
        self.target_fixture.sut.diagnose_hard_reset()

    @tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
    def start(self, delay_seconds=0):
        """Start DataRouter after optional delay."""
        if delay_seconds > 0:
            time.sleep(delay_seconds)

        with self.target_fixture.sut.ssh() as ssh:
            SshCommand(ssh, "cd /bmw/platform/opt/datarouter/ && export AMSR_DISABLE_INTEGRITY_CHECK=1 && /ifs/bin/on -T datarouter_t -u 1038:1054,1036,10007,3020 -A nonroot,allow,pathspace ./bin/datarouter --no_adaptive_runtime &")

        assert is_app_running(self.target_fixture, "datarouter"), f"Failed to start datarouter"

    @tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
    def stop(self):
        """Stop DataRouter and cleanup."""
        with self.target_fixture.sut.ssh() as ssh:
            execute_command(ssh, "slay datarouter")
        time.sleep(1)
        assert not is_app_running(self.target_fixture, "datarouter"), f"Failed to slay datarouter"


@xtf_retry()
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
@pytest.mark.metadata(description="Verifies DataRouter's ability to handle delayed startup scenario where applications "
                      "attempt to connect and log before DataRouter is available. The test starts a logging application first "
                      "which will wait for DataRouter connection, then delays DataRouter startup by several seconds. "
                      "During this delay period, the application should queue log messages and maintain connection attempts. "
                      "When DataRouter finally starts, it should successfully establish connection with the waiting application "
                      "and retrieve all queued log messages. This scenario validates the robustness of the logging system "
                      "when DataRouter experiences delayed startup or restart scenarios.",
                      verifies=[1632647],
                      domain="Performance and Stability",
                      status="Ready",
                      ASIL="QM",
                      priority=3,
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements")
def test_logging_after_delayed_dr_start(target_fixture):

    dlt = DltWindow()
    with DataRouterManager(target_fixture) as dr_manager:
        dr_manager.stop()
        with target_fixture.sut.ssh() as ssh:
            with dlt.record():
                with RemoteBinaryExecution(target_fixture, APP_PATH, APP_BIN, MINIMAL_LOGGING_CONFIG) as exe_remote:
                    exe_remote.run_in_background(ssh, args=f"--sbs {SLEEP_BEFORE_SHUTTINGDOWN}")
                    dr_manager.start(delay_seconds=2)
                    time.sleep(5)

    dlt.load()
    query_dict = dict(payload_decoded= re.compile(f".*Accumulated frontend exectution time:.*"))

    result = dlt.find(query=query_dict)

    dlt.clear()
    assert (
        len(result) > 0
    ), f"Couldn't find logs from dlt_generator"
