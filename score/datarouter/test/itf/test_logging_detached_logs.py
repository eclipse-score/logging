
import re
import time
import logging
import pytest
import tenacity
from itf.actions.remote_binary_execution import RemoteBinaryExecution
from xtf_common.retry import xtf_retry
from xtf_common.process.dlt_window import DltWindow

logger = logging.getLogger(__name__)

# config section
LOG_RING_BUFFER_SIZE_CONFIG = 2*1024*1024  #  2MB

#consts
COMMON_MSG_PART = "default message text for example log generating application"

LGGG_APP_ID = "LGGG"
APP_PATH = "platform/aas/pas/logging/test/itf/"
APP_BIN = "dlt_generator"

MINIMAL_LOGGING_CONFIG = {
    "appId": LGGG_APP_ID,
    "appDesc": "Test Log Generator",
    "ringBufferSize": LOG_RING_BUFFER_SIZE_CONFIG,
    "logLevel": "kVerbose",
    "logLevelThresholdConsole": "kVerbose",
    "logMode": "kSystem",
}


@xtf_retry()
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
@pytest.mark.metadata(description="Verifies DataRouter's ability to handle detached logs from applications that exit immediately after logging. "
                      "The test captures logs from an application that terminates right after putting log data into shared memory, "
                      "without waiting for DataRouter synchronization. When the application exits, the shared memory is unlinked "
                      "from the client side but remains available for DataRouter to read. DataRouter must detect this condition, "
                      "switch to detached mode, and successfully retrieve all remaining logs from the terminated application. "
                      "This scenario validates that log data is not lost when applications exit abruptly after logging.",
                      testType="Verification of the control flow and data flow",
                      derivationTechnique="Analyzing architecture and design")
def test_logging_detached_logs(target_fixture):


    dlt = DltWindow()
    with dlt.record():
        with RemoteBinaryExecution(target_fixture, APP_PATH, APP_BIN, MINIMAL_LOGGING_CONFIG) as exe_remote:
            exe_remote.run_and_check_exit_code(args=f"--sleep_before_shutdown 0")  # No wait before shutdown, immediate exit after logging
        time.sleep(5)

    dlt.load()
    query_dict = dict(payload_decoded= re.compile(f".*Accumulated frontend exectution time:.*"))

    result = dlt.find(query=query_dict)

    dlt.clear()
    assert (
        len(result) > 0
    ), f"Couldn't find logs from dlt_generator"
