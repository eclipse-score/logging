# pylint: disable=too-many-locals
"""
Execute the logging somke tests with ITF
"""
import logging
import re
import time
import os
import tempfile
import pytest
from xtf_common.process.dlt_window import DltWindow
import dlt
from itf.com.ssh import execute_command
from itf.actions.actions import get_app_pids
from itf.actions.remote_binary_execution import RemoteBinaryExecution
from itf.com.ssh_command import SshCommand

logger = logging.getLogger(__name__)

LOGC_CTX_ID = "LOGG"
LGGG_APP_ID = "LISI"
COMMON_MSG_PART = "default message text for example log generating application"
LOG_GENERATION_ITERATIONS = 2000
_APP_PATH = "./platform/aas/pas/logging/test/itf"
_LOGGING_CONFIG = f"./platform/aas/pas/logging/test/itf/safe_ipc_fault_app/etc/logging.json"

def _count_message_occurrence(log_file_local_path, app_id, ctx_id, msg):
    if not os.path.isfile(log_file_local_path):
        raise FileNotFoundError(f"DLT file does not exist : {log_file_local_path}")
    try:
        dlt_file = dlt.load(log_file_local_path, None)
    except IOError as io_error:
        raise RuntimeError(f"Loading DLT file failed: {log_file_local_path}") from io_error

    count = 0
    message_iterator = iter(dlt_file)
    try:
        for message in message_iterator:
            if message.use_extended_header:
                if (app_id is None or message.apid.decode("ascii") == app_id) and \
                        (ctx_id is None or message.ctid.decode("ascii") == ctx_id):
                    payload = message.payload_decoded
                    if isinstance(payload, bytes):
                        payload = payload.decode("ascii", errors='ignore')
                    if re.match(r'(.*)'+msg+'(.*)', payload):
                        count += 1
        return count
    except IOError:  #  capture case of empty file
        #  continue tests despite exception occurrence
        logger.exception(f"Empty DLT file exception has description")
        return 0

@pytest.mark.metadata(description="start faulty app then dummy app to make sure that datarouter not stuck to make sure that\
                                    no infinite error with missing shared memroy file when client exit immediately after startup only\
                                    data router will try to connect with the client for only one second",
                      verifies=[1633539],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
                      status="Ready",
                      ASIL="QM",
                      priority=3)
@pytest.mark.skip(reason="Temporary disabled, should be enabled when Ticket-132779 would be implemented")
def test_datarouter_when_app_disconnect_immediate(target_fixture):
    with target_fixture.sut.ssh() as ssh:
        # killing datarouter then starting it again to record error logs.
        datarouter_pid = get_app_pids(target_fixture, "datarouter")[0]
        assert execute_command(ssh, f"slay -s SIGKILL {datarouter_pid} &", timeout=1) == 0
        SshCommand(ssh, "cd /opt/datarouter/ && /ifs/bin/on -T datarouter_t -u 1038:1036  ./bin/datarouter --no_adaptive_runtime  > /dev/null 2>&1 & ")
        # starting faulty app right after spawning data router to be able to reproduce connection issue.
        with RemoteBinaryExecution(target_fixture, _APP_PATH, "fault_app", etc_files=[_LOGGING_CONFIG]) as exe_remote:
            exe_remote.run_and_check_exit_code()

    # starting dummy logging app right after faulty app and make sure to capture logs
    with tempfile.NamedTemporaryFile() as tmp_file:
        dltwindow = DltWindow(dlt_file=tmp_file.name)
        with dltwindow.record():
            with RemoteBinaryExecution(target_fixture, _APP_PATH, "dlt_generator", etc_files=[_LOGGING_CONFIG]) as exe_remote:
                exe_remote.run_and_check_exit_code(args=f"--it {LOG_GENERATION_ITERATIONS}")
            time.sleep(10)

        dltfile = dltwindow.get_dlt_file_path()
        occurrences = _count_message_occurrence(dltfile, LGGG_APP_ID, LOGC_CTX_ID, COMMON_MSG_PART)
        assert occurrences > 0
