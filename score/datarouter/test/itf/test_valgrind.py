# pylint: disable=unused-import
import logging
from time import sleep
import os

import pytest

from pas_common import dm_verity_fixture # pylint: disable=unused-import
from valgrind_runner import ValgrindRunner, valgrind_test_fixture # pylint: disable=unused-import
from itf.com.ssh import execute_command, execute_command_output
from itf.actions.mount_overlay import writable_overlay_fixture # pylint: disable=unused-import
from itf.actions.actions import ensure_tmpfs_exists
from itf.actions.actions import get_app_pids
from itf.actions.remote_binary_execution import RemoteBinaryExecution
logger = logging.getLogger(__name__)

@pytest.mark.hw_only
@pytest.mark.supported_ecu_family(["ipnext"])
@pytest.mark.supported_os(["qnx7", "qnx8"])
@pytest.mark.metadata(
    description="Run datarouter under Valgrind and ensure no memory errors are reported",
    verifies=[20615425],
    domain="Performance and Stability",
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    ASIL="QM",
    status="Ready",
    priority=3)
def test_valgrind_datarouter(target_fixture, dm_verity_fixture, valgrind_test_fixture):

    app = "bin/datarouter"
    working_dir = "/bmw/platform/opt/datarouter/"
    args = []
    env = ["AMSR_DISABLE_INTEGRITY_CHECK=1"]
    additional_valgrind_args = ["--gen-suppressions=all", "--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes"]
    with target_fixture.sut.ssh() as ssh:
        with ValgrindRunner(target_fixture=target_fixture, ssh_connection=ssh,
                            app=working_dir+app,
                            working_dir=working_dir,
                            args=args,
                            env=env,
                            additional_valgrind_args=additional_valgrind_args,
                            valgrind_suppressions_file=True,
                            stop_running_app=True
                            ):
            sleep(10)

ARA_LOG_APP_PATH = "platform/aas/pas/logging/test/itf/dynamic_allocation_apps/"
ARA_LOG_APP = "ara_log_allocations"
LOGGING_CONFIG = "etc/logging.json"
@pytest.mark.hw_only
@pytest.mark.supported_ecu_family(["ipnext"])
@pytest.mark.supported_os(["qnx7", "qnx8"])
@pytest.mark.metadata(
    description="Run ara_log_allocations test app under Valgrind and ensure no memory errors are reported",
    verifies=[20615425],
    domain="Performance and Stability",
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    ASIL="QM",
    status="Ready",
    priority=3)
def test_valgrind_with_dynamic_allocations_app(target_fixture, dm_verity_fixture, valgrind_test_fixture, target_sftp_connection, target_ssh_connection):
    tmpfs = ensure_tmpfs_exists(target_fixture)
    target_sftp_connection.upload(f"{ARA_LOG_APP_PATH}/{ARA_LOG_APP}", f"{tmpfs}/{ARA_LOG_APP}")
    target_sftp_connection.upload(f"{ARA_LOG_APP_PATH}/{LOGGING_CONFIG}", f"{tmpfs}/logging.json")
    assert execute_command(target_ssh_connection, f"chmod +x {tmpfs}/{ARA_LOG_APP}") == 0
    the_app = str(str(tmpfs)+"/"+ARA_LOG_APP)

    working_dir = "/"
    args = []
    env = []
    additional_valgrind_args = ["--gen-suppressions=all", "--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes"]
    with target_fixture.sut.ssh() as ssh:
        with ValgrindRunner(target_fixture=target_fixture, ssh_connection=ssh,
                            app=the_app,
                            working_dir=working_dir,
                            args=args,
                            env=env,
                            additional_valgrind_args=additional_valgrind_args,
                            valgrind_suppressions_file=True,
                            stop_running_app=True
                            ):
            sleep(10)


CROSS_LOCKING_PATH = "platform/aas/pas/logging/test/itf/logging_cross_locking_app/"
CROSS_LOCKING_APP = "cross_locking_app"
@pytest.mark.hw_only
@pytest.mark.supported_ecu_family(["ipnext"])
@pytest.mark.supported_os(["qnx7", "qnx8"])
@pytest.mark.metadata(
    description="Run cross_locking_app under Valgrind and ensure no memory errors are reported",
    verifies=[20615425],
    domain="Performance and Stability",
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
    ASIL="QM",
    status="Ready",
    priority=3)
def test_valgrind_with_cross_locking_app(target_fixture, dm_verity_fixture, valgrind_test_fixture, target_sftp_connection, target_ssh_connection):
    tmpfs = ensure_tmpfs_exists(target_fixture)
    target_sftp_connection.upload(f"{CROSS_LOCKING_PATH}/{CROSS_LOCKING_APP}", f"{tmpfs}/{CROSS_LOCKING_APP}")
    target_sftp_connection.upload(f"{CROSS_LOCKING_PATH}/{LOGGING_CONFIG}", f"{tmpfs}/logging.json")
    assert execute_command(target_ssh_connection, f"chmod +x {tmpfs}/{CROSS_LOCKING_APP}") == 0
    the_app = str(str(tmpfs)+"/"+CROSS_LOCKING_APP)

    working_dir = "/"
    args = []
    env = []
    additional_valgrind_args = ["--gen-suppressions=all", "--leak-check=full", "--show-leak-kinds=all", "--track-origins=yes"]
    with target_fixture.sut.ssh() as ssh:
        with ValgrindRunner(target_fixture=target_fixture, ssh_connection=ssh,
                            app=the_app,
                            working_dir=working_dir,
                            args=args,
                            env=env,
                            additional_valgrind_args=additional_valgrind_args,
                            valgrind_suppressions_file=True,
                            stop_running_app=False
                            ):
            sleep(10)
