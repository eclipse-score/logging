# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

import re

from attribute_plugin import add_test_properties

APPS_PATH = "/opt/test_apps/dynamic_allocation_apps"

MW_LOG_GOLDEN_APP = "mw_log_golden_app"
MW_LOG_APP = "mw_log_allocations"

NON_VERBOSE_APP = "non_verbose_allocations"
NON_VERBOSE_GOLDEN_APP = "non_verbose_golden_app"
LOGGING_CONFIG = "etc/logging.json"

MALLOC_PROFILER_LIB = "libmalloc-profiler.so"


def construct_allocations(line, given_list):
    substr = ","
    listx = [1, 3, 5, 6]
    occurnace_list = []
    for occurrence in listx:
        inilist = [m.start() for m in re.finditer(substr, line)]
        if len(inilist) >= occurrence:
            occurnace_list.append(inilist[occurrence - 1])
        else:
            pass
    alloc = (
        line[occurnace_list[0] + 1 : occurnace_list[1]]
        + line[occurnace_list[2] : occurnace_list[3]]
    )
    if alloc.strip().startswith("ma,"):
        given_list.append(alloc)


def _execute_output(target, command):
    exit_code, out = target.execute(command)
    return exit_code, out.decode(errors="replace").splitlines()


def upload_apps_and_run(target, app_name):
    allocations_list = []
    _, lines = _execute_output(target, "ls /dev/shmem | grep profiler")
    if len(lines) > 0:
        fname = lines[0].strip()
        assert target.execute(f"rm /dev/shmem/{fname}")[0] == 0
    assert target.execute(f"cd {APPS_PATH} && ./bin/{app_name}")[0] == 0
    assert (
        target.execute(
            f"cd {APPS_PATH} && MW_LOG_CONFIG_FILE={APPS_PATH}/{LOGGING_CONFIG} PROFILER_ENABLED=1 LD_PRELOAD={APPS_PATH}/lib/{MALLOC_PROFILER_LIB} ./bin/{app_name} > /dev/null 2>&1 "
        )[0]
        == 0
    )

    _, lines = _execute_output(target, "ls /dev/shmem | grep profiler")
    assert len(lines) > 0
    fname = lines[0].strip()
    _, lines = _execute_output(target, f"cat /dev/shmem/{fname}")
    for line in lines:
        if lines[0] != line:
            if line != lines[len(lines) - 1]:
                construct_allocations(line.strip(), allocations_list)
    return allocations_list


@add_test_properties(
    fully_verifies=["comp_req__log__local_allocation_strategy"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_mw_log_allocations(target, datarouter_on_target):
    """Check that all mw log public APIs don't allocate memory after mw log initialization."""
    mw_log_golden_app_list = upload_apps_and_run(target, MW_LOG_GOLDEN_APP)
    mw_log_public_apis_list = upload_apps_and_run(target, MW_LOG_APP)
    mw_log_golden_app_list_sorted = sorted(mw_log_golden_app_list)
    mw_log_public_apis_list_sorted = sorted(mw_log_public_apis_list)
    assert mw_log_golden_app_list_sorted == mw_log_public_apis_list_sorted


@add_test_properties(
    fully_verifies=["comp_req__log__local_allocation_strategy"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
def test_non_verbose_allocations(target, datarouter_on_target):
    """Check that all non verbose public APIs don't allocate memory after initialization."""
    non_verbose_golden_app_list = upload_apps_and_run(target, NON_VERBOSE_GOLDEN_APP)
    non_verbose_public_apis_list = upload_apps_and_run(target, NON_VERBOSE_APP)
    non_verbose_golden_app_list_sorted = sorted(non_verbose_golden_app_list)
    non_verbose_public_apis_list_sorted = sorted(non_verbose_public_apis_list)
    assert non_verbose_golden_app_list_sorted == non_verbose_public_apis_list_sorted
