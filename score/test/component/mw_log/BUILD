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

load("//score/test/component:defs.bzl", "py_logging_itf_test")

py_logging_itf_test(
    name = "test_mw_log",
    srcs = ["test_mw_log.py"],
    filesystem = "//score/test/component/logging_app:logging_app_filesystem",
    deps = [
        "@score_itf//third_party/python_dlt",
    ],
)

py_logging_itf_test(
    name = "test_mw_log_kfile",
    srcs = ["test_mw_log_kfile.py"],
    filesystem = "//score/test/component/dlt_generator_app:dlt_generator_filesystem",
    deps = [
        "@score_itf//third_party/python_dlt",
    ],
)

py_logging_itf_test(
    name = "test_mw_log_filters",
    srcs = ["test_mw_log_filters.py"],
    filesystem = "//score/test/component/filters_app:mwfiltertest_filesystem",
)
