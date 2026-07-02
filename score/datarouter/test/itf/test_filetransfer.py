# pylint: disable=too-many-locals
"""
Execute the filetransfer test with ITF
"""
import time
import os
import tempfile
import tenacity
import pytest  # pylint: disable=unused-import
from extract_files_frm_dlt import ExtractFileFromDlt # pylint: disable=import-error
from itf.actions.remote_binary_execution import RemoteBinaryExecution
from itf.com.ssh import execute_command
from xtf_common.process.dlt_window import DltWindow

@pytest.mark.metadata(description="Datarouterconf shall request DLT file transfer for persistent log file",
                      verifies=[5268601],
                      testType="Requirements-based test",
                      derivationTechnique="Analysis of requirements",
                      ASIL="QM",
                      status="Ready",
                      priority=3)
@tenacity.retry(wait=tenacity.wait_fixed(1), stop=tenacity.stop_after_attempt(5), reraise=True)
def test_file_transfer(target_fixture):
    """
    Test the file transfer, file creation from dlt file.
    Remove the previously created  {tmpfs}/filetransfer dir
    Creates {tmpfs}/filetransfer/bin
    Execute the filetransfer binary
    Assert for failure  cases
    """
    with tempfile.NamedTemporaryFile() as tmp_file:
        dltwindow = DltWindow(dlt_file=tmp_file.name)
        path_to_test_core_file_on_host = \
                "platform/aas/pas/logging/test/sct/filetransfer_over_dlt/context.3155760015.MsmStateSetterGetter.887.txt"
        path_to_test_core_file_on_target = f"/tmp/context.3155760015.MsmStateSetterGetter.887.txt"
        with target_fixture.sut.ssh() as ssh:
            with target_fixture.sut.sftp(ssh) as sftp:
                #  test binary executable expects hardcoded file in specific directory
                sftp.upload(f"{path_to_test_core_file_on_host}", f"{path_to_test_core_file_on_target}")

        with dltwindow.record():
            app_path = "platform/aas/pas/logging/test/itf/"
            app_bin = "filetransfer_app"
            #  ==========  Execute the transfer
            with RemoteBinaryExecution(target_fixture, app_path, app_bin) as exe_remote:
                exe_remote.run_and_check_exit_code()
            #  ==========
            time.sleep(15)

        with target_fixture.sut.ssh() as ssh:
            assert execute_command(ssh, f"rm -r {path_to_test_core_file_on_target}") == 0

        dltfile = dltwindow.get_dlt_file_path()
        if not os.path.isfile(dltfile):
            raise FileNotFoundError(f"DLT file does not exist : {dltfile}")
        extract_dlt_files = ExtractFileFromDlt(dltfile, path_to_create_file=app_path, path_to_origin_file=path_to_test_core_file_on_host, total_no_of_files=50, is_sct_itf=True, appid="FTEA", ctxid="FMSG")
        extract_dlt_files.extract_files_from_dlt()
