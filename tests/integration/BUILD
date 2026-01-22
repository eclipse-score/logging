# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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

load("@score_itf//:defs.bzl", "py_itf_test")

package(default_testonly = True)

py_itf_test(
    name = "test_datarouter_dlt",
    srcs = [
        "__init__.py",
        "conftest.py",
        "test_datarouter_dlt.py",
    ],
    args = [
        "--ecu=s_core_ecu_qemu_bridge_network",
        "--target_config=$(location @score_itf//config)",
        "--hw", # Required even if we run the test against a QEMU target
    ],
    data = [
        "@score_itf//config",
    ],
    plugins = [
        "itf.plugins.base.base_plugin",
    ],
)
