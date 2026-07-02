# pylint: disable=too-many-locals
"""
Execute the logging somke tests with ITF
"""
import logging
import time
import os
import shutil
import machine_state
import pytest
from itf.actions.remote_binary_execution import RemoteBinaryExecution
from xtf_common.retry import xtf_retry
from pas_common import count_dlt_message_occurrence

logger = logging.getLogger(__name__)

# config section:
DLT_KFILE_LOG_GENERATION_ITERATIONS = 12

TIMEOUT_WAIT_LOGGING_CONFIG_AND_MESSAGE_RECEPTION_SEC = 3

#consts
COMMON_MSG_PART = "default message text for example log generating application"
LOGG_CTX_ID = "LOGG"
LGGG_APP_ID = "LGGG"

LOG_OCCURRENCE_COUNT = {
    "OFF": 0,
    "FATAL": 1,
    "ERROR": 2,
    "WARNING": 3,
    "INFO": 4,
    "DEBUG": 5,
    "VERBOSE": 6,
}

MINIMAL_KFILE_LOGGING_CONFIG = {
    "appId": LGGG_APP_ID,
    "appDesc": "Test File Log Generator",
    "logLevel": "kVerbose",
    "logLevelThresholdConsole": "kError",
    "logMode": "kFile",
}

@pytest.fixture(scope="function")
def upload_kfile_app_and_etc_fixture(target_fixture):
    # prepare directories and generate config:
    app_path = "platform/aas/pas/logging/test/itf/"
    app_bin = "dlt_generator"

    logger.info(f"upload_kfile_app_and_etc_fixture context init")
    with RemoteBinaryExecution(target_fixture, app_path, app_bin, MINIMAL_KFILE_LOGGING_CONFIG) as exe_remote:
        yield exe_remote

    logger.info(f"Exit upload_kfile_app_and_etc_fixture(target_fixture):")

def _ensure_machine_state_running(target_fixture):
    target_fixture.sut.diagnose_ping()
    # Wait until Running machine state first to ensure that all diag jobs are available.
    with target_fixture.sut.uds() as uds:
        machine_state.wait_until_machine_state(uds)

def _check_remote_file_exist(sftp, remote_dir_path, remote_file_name):
    remote_list = sftp.list_dirs_and_files_name(remote_dir_path)
    return bool(remote_file_name in remote_list)

def _download_remote_dlt_log_file(sftp, app_name, local_path):
    file_name = app_name+".dlt"
    tmp_local_path_file = local_path+"/"+file_name
    remote_file_path = "/tmp/"+file_name

    assert _check_remote_file_exist(sftp, "/tmp/", file_name), f"file /tmp/{file_name} not available for {app_name}"
    create_dir_and_download_file(sftp, tmp_local_path_file, remote_file_path)
    return tmp_local_path_file

def create_dir_and_download_file(sftp, tmp_local_file_path, remote_file_path):
    if os.path.isdir(tmp_local_file_path):
        shutil.rmtree(tmp_local_file_path, ignore_errors=True)
    local_dir_path = os.path.dirname(tmp_local_file_path)
    if not os.path.isdir(local_dir_path):
        os.makedirs(local_dir_path)
    sftp.download(remote_file_path, tmp_local_file_path)

@pytest.mark.metadata(
    description="Log messages shall be placed in DLT file locate on target filesystem",
    verifies=[1633236],
    domain="Performance and Stability",
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    ASIL="QM",
    status="Ready",
    priority=3)
@xtf_retry()
@pytest.mark.failure_fatal
def test_dlt_kfile_logging(target_fixture, upload_kfile_app_and_etc_fixture):
    """
    Test DLT logging to file
    Assert for failure cases
    """
    _ensure_machine_state_running(target_fixture)
    upload_kfile_app_and_etc_fixture.run_and_check_exit_code(args=f"--it {DLT_KFILE_LOG_GENERATION_ITERATIONS}")
    time.sleep(TIMEOUT_WAIT_LOGGING_CONFIG_AND_MESSAGE_RECEPTION_SEC) # Wait DLT messages to be written

    #  Download the DTL file from the target:
    with target_fixture.sut.sftp() as sftp:
        local_dlt_file = _download_remote_dlt_log_file(sftp, LGGG_APP_ID, "./tmp_files/")
        occurrences = count_dlt_message_occurrence(local_dlt_file, LGGG_APP_ID, LOGG_CTX_ID, COMMON_MSG_PART)
        logger.info(f"Local DLT file: {local_dlt_file}, occurrences: {occurrences}")
        assert occurrences == DLT_KFILE_LOG_GENERATION_ITERATIONS*LOG_OCCURRENCE_COUNT["VERBOSE"]
