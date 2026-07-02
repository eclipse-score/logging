# pylint: disable=too-many-locals
"""
Execute the logging somke tests with ITF
"""
import re
import logging
import pytest
import tenacity
from itf.actions.remote_binary_execution import RemoteBinaryExecution
from xtf_common.retry import xtf_retry

logger = logging.getLogger(__name__)

# config section
LOG_RING_BUFFER_SIZE_CONFIG = 2*1024*1024  #  2MB

#consts
COMMON_MSG_PART = "default message text for example log generating application"
LOG_GENERATION_ITERATIONS = 10
LOG_EXPECTED_MESSAGES_PER_ITERATION = 6

LGGG_APP_ID = "LGGG"

CONSOLE_LOGGING_CONFIG = {
    "appId": LGGG_APP_ID,
    "appDesc": "Test Log Generator",
    "ringBufferSize": LOG_RING_BUFFER_SIZE_CONFIG,
    "logLevel": "kVerbose",
    "logLevelThresholdConsole": "kVerbose",
    "logMode": "kConsole",
}

@pytest.fixture(scope="module")
def upload_app_console_and_etc_fixture(target_fixture):
    # prepare directories and generate config:
    app_path = "platform/aas/pas/logging/test/itf/"
    app_bin = "dlt_generator"

    logger.info(f"upload_app_console_and_etc_fixture context init")
    with RemoteBinaryExecution(target_fixture, app_path, app_bin, CONSOLE_LOGGING_CONFIG) as exe_remote:
        yield exe_remote

    logger.info(f"Exit upload_app_console_and_etc_fixture(target_fixture):")

@xtf_retry()
@pytest.mark.failure_fatal
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
@pytest.mark.metadata(
    description="ara::log shall emit expected console output (std::ostream) for the log generator application",
    verifies=[1633236],
    domain="Performance and Stability",
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    ASIL="QM",
    status="Ready",
    priority=3)
def test_console_logging(upload_app_console_and_etc_fixture):
    """
    Test console output (std::ostream) for test application
    Assert for failure cases
    """
    exec_result = upload_app_console_and_etc_fixture.run(args=f"--it {LOG_GENERATION_ITERATIONS}")
    std_out_data = exec_result.get_stdout_bytes().decode('ascii')

    query_string = re.compile(COMMON_MSG_PART)
    result_count = len(re.findall(query_string, std_out_data))

    assert result_count == LOG_GENERATION_ITERATIONS * LOG_EXPECTED_MESSAGES_PER_ITERATION
