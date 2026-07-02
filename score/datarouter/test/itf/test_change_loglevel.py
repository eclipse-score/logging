import logging
import time
import pytest
import dlt

from xtf_common.process.dlt_window import DltWindow
from itf.actions.remote_binary_execution import RemoteBinaryExecution

from pas_common import kill_app_with_signal

logger = logging.getLogger(__name__)

APPS_PATH = "platform/aas/pas/logging/test/itf/change_loglevel"
APP_NAME = "change_log_level_test_app"
CLL_APP_ID = "CLL"
CLL_CTXT_ID = "DFLT"

OFF     = 0
FATAL   = 1
ERROR   = 2
WARN    = 3
INFO    = 4
DEBUG   = 5
VERBOSE = 6

# variable to adjust application loglevel
REQUIRED_LOGLEVEL = "kDebug"

# Create a dictionary to map string identifiers to their corresponding values
loglevel_map = {
    "kOff": OFF,
    "kFatal": FATAL,
    "kError": ERROR,
    "kWarn": WARN,
    "kInfo": INFO,
    "kDebug": DEBUG,
    "kVerbose": VERBOSE
}

# Config dict passed directly to RemoteBinaryExecution — replaces the on-target
# cp/sed/cp dance.  The framework serialises this to JSON and uploads it,
# so /persistent is never touched.
_MODIFIED_LOGGING_CONFIG = {
    "appId": CLL_APP_ID,
    "appDesc": "Change LogLevel Test Application",
    "logLevel": REQUIRED_LOGLEVEL,
    "logMode": "kRemote|kConsole|kFile|kSystem",
}

def capture_and_parse_dlt(ssh, exe_remote):
    logger.info("starting dlt recording ! ")
    dlt_window = DltWindow(dlt_filter=f"{CLL_APP_ID} {CLL_CTXT_ID}")
    loglevel = 0
    with dlt_window.record():
        logger.info("changeloglevel app is running!")
        exe_remote.run_in_background(ssh)
        time.sleep(5)
    dlt_file = dlt.load(filename=dlt_window.get_dlt_file_path(), filters=[(CLL_APP_ID, "")])
    for msg in dlt_file:
        if msg.apid.decode("ascii") == CLL_APP_ID:
            loglevel +=1
    return loglevel

@pytest.mark.supported_os(["qnx7", "qnx8"])
@pytest.mark.metadata(description="For debug protocols that support log levels, it shall be possible to change the log level\
                                   for each software stack in order to adjust the amount of debug data that is streamed out.",
                      status="Ready",
                      verifies=[4993239],
                      domain="Performance and Stability",
                      ASIL="QM",
                      priority=3,
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements")
def test_change_loglevel(target_fixture):
    logger.info(f"Starting ITF testcase for changing loglevel")
    current_loglevel = 0
    with target_fixture.sut.ssh() as ssh:
        with RemoteBinaryExecution(target_fixture, APPS_PATH, APP_NAME, _MODIFIED_LOGGING_CONFIG) as exe_remote:
            kill_app_with_signal(target_fixture, APP_NAME, "SIGTERM")
            current_loglevel = capture_and_parse_dlt(ssh, exe_remote)
    assert loglevel_map[REQUIRED_LOGLEVEL] == current_loglevel
    logger.info("Application loglevel is changed sucessfully!")
