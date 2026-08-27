# pylint: disable=redefined-outer-name, unused-import

import re

import pytest
from itf.actions.actions import ensure_tmpfs_exists
from itf.com.ssh import execute_command, execute_command_output
from itf.target.os import OperatingSystem

APPS_PATH = "platform/aas/pas/logging/test/itf/dynamic_allocation_apps"
ARA_LOG_GOLDEN_APP = "ara_log_golden_app"
ARA_LOG_APP = "ara_log_allocations"

MW_LOG_GOLDEN_APP = "mw_log_golden_app"
MW_LOG_APP = "mw_log_allocations"

NON_VERBOSE_APP = "non_verbose_allocations"
NON_VERBOSE_GOLDEN_APP = "non_verbose_golden_app"
LOGGING_CONFIG = "etc/logging.json"

MALLOC_PROFILER_PATH = "platform/aas/performance/test/tools/malloc_profiler"
MALLOC_PROFILER_LIB = "libmalloc-profiler.so"


def construct_allocations(line, given_list):
    # this function to take an empty list and fill it with the allocations without free statments
    # output will be informat: ma,calloc,32 saved in a list
    substr = ","
    listx = [1, 3, 5, 6]
    occurnace_list = []
    for occurrence in listx:
        # Finding nth occurrence of substring
        inilist = [m.start() for m in re.finditer(substr, line)]
        if len(inilist) >= occurrence:
            occurnace_list.append(inilist[occurrence - 1])
        else:
            pass
    alloc = (
        line[occurnace_list[0] + 1 : occurnace_list[1]]
        + line[occurnace_list[2] : occurnace_list[3]]
    )
    if alloc.strip().find("fr") != -1:
        pass
    else:
        given_list.append(alloc)


def upload_apps_and_run(
    target_fixture, target_ssh_connection, target_sftp_connection, app_name
):
    allocations_list = []
    _, lines, _ = execute_command_output(
        target_ssh_connection, f"ls /dev/shmem | grep profiler", verbose=False
    )
    if len(lines) > 0:
        fname = lines[0].strip()
        assert execute_command(target_ssh_connection, f"rm /dev/shmem/{fname}") == 0
    tmpfs = ensure_tmpfs_exists(target_fixture)
    target_sftp_connection.upload(f"{APPS_PATH}/{app_name}", f"{tmpfs}/{app_name}")
    target_sftp_connection.upload(
        f"{APPS_PATH}/{LOGGING_CONFIG}", f"{tmpfs}/logging.json"
    )
    target_sftp_connection.upload(
        f"{MALLOC_PROFILER_PATH}/{MALLOC_PROFILER_LIB}",
        f"{tmpfs}/{MALLOC_PROFILER_LIB}",
    )
    assert execute_command(target_ssh_connection, f"chmod +x {tmpfs}/{app_name}") == 0
    assert execute_command(target_ssh_connection, f"{tmpfs}/{app_name}") == 0
    assert (
        execute_command(
            target_ssh_connection,
            f" MW_LOG_CONFIG_FILE={tmpfs}/logging.json PROFILER_ENABLED=1 LD_PRELOAD={tmpfs}/libmalloc-profiler.so {tmpfs}/{app_name} > /dev/null 2>&1 ",
        )
        == 0
    )

    _, lines, _ = execute_command_output(
        target_ssh_connection, f"ls /dev/shmem | grep profiler", verbose=False
    )
    assert len(lines) > 0
    fname = lines[0].strip()
    _, lines, _ = execute_command_output(
        target_ssh_connection, f"cat /dev/shmem/{fname}", verbose=False
    )
    for line in lines:
        if lines[0] != line:
            if line != lines[len(lines) - 1]:
                construct_allocations(line.strip(), allocations_list)
    return allocations_list


@pytest.mark.run(order=1)
@pytest.mark.supported_os(["qnx"])
@pytest.mark.metadata(
    description="Verifies if ara::log uses local allocators to avoid using global heap. Global heap allocation (if any) shall be limited to initialization phase of application lifecycle.",
    testType="Resource usage evaluation",
    derivationTechnique="Analyzing architecture and design",
)
def test_ara_log_allocations(
    target_fixture, target_ssh_connection, target_sftp_connection
):
    ara_log_golden_app_list = upload_apps_and_run(
        target_fixture,
        target_ssh_connection,
        target_sftp_connection,
        ARA_LOG_GOLDEN_APP,
    )
    ara_log_public_apis_list = upload_apps_and_run(
        target_fixture, target_ssh_connection, target_sftp_connection, ARA_LOG_APP
    )
    ara_log_golden_app_list_sorted = sorted(ara_log_golden_app_list)
    ara_log_public_apis_list_sorted = sorted(ara_log_public_apis_list)
    assert ara_log_golden_app_list_sorted == ara_log_public_apis_list_sorted


@pytest.mark.run(order=2)
@pytest.mark.supported_os(["qnx"])
@pytest.mark.metadata(
    description="check that all mw log public APIs don't allocate memory after mw log initialization",
    verifies=[861534, 861550],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
)
def test_mw_log_allocations(
    target_fixture, target_ssh_connection, target_sftp_connection
):
    mw_log_golden_app_list = upload_apps_and_run(
        target_fixture, target_ssh_connection, target_sftp_connection, MW_LOG_GOLDEN_APP
    )
    mw_log_public_apis_list = upload_apps_and_run(
        target_fixture, target_ssh_connection, target_sftp_connection, MW_LOG_APP
    )
    mw_log_golden_app_list_sorted = sorted(mw_log_golden_app_list)
    mw_log_public_apis_list_sorted = sorted(mw_log_public_apis_list)
    assert mw_log_golden_app_list_sorted == mw_log_public_apis_list_sorted


@pytest.mark.run(order=3)
@pytest.mark.supported_os(["qnx"])
@pytest.mark.metadata(
    description="check that all non verbose public APIs don't allocate memory after initialization",
    verifies=[861534, 861550],
    testType="Requirements-based test",
    derivationTechnique="Analysis of requirements",
)
def test_non_verbose_allocations(
    target_fixture, target_ssh_connection, target_sftp_connection
):
    non_verbose_golden_app_list = upload_apps_and_run(
        target_fixture,
        target_ssh_connection,
        target_sftp_connection,
        NON_VERBOSE_GOLDEN_APP,
    )
    non_verbose_public_apis_list = upload_apps_and_run(
        target_fixture, target_ssh_connection, target_sftp_connection, NON_VERBOSE_APP
    )
    non_verbose_golden_app_list_sorted = sorted(non_verbose_golden_app_list)
    non_verbose_public_apis_list_sorted = sorted(non_verbose_public_apis_list)
    assert non_verbose_golden_app_list_sorted == non_verbose_public_apis_list_sorted
