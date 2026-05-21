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

load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("//score/test/itf:defs.bzl", "py_logging_itf_test")

cc_binary(
    name = "dlt_generator",
    srcs = ["dlt_generator.cpp"],
    deps = ["//score/mw/log"],
)

pkg_tar(
    name = "dlt_generator_bin_pkg",
    srcs = [":dlt_generator"],
    package_dir = "opt/test_apps/dlt_generator/bin",
)

pkg_tar(
    name = "dlt_generator_config_pkg",
    srcs = ["etc/logging.json"],
    package_dir = "opt/test_apps/dlt_generator/etc",
)

pkg_tar(
    name = "dlt_generator_filesystem",
    deps = [
        ":dlt_generator_bin_pkg",
        ":dlt_generator_config_pkg",
    ],
)

py_logging_itf_test(
    name = "test_mw_log_kfile",
    srcs = ["test_mw_log_kfile.py"],
    filesystem = ":dlt_generator_filesystem",
    deps = [
        "//score/test/itf:dlt_parser",
        "@score_itf//third_party/python_dlt",
    ],
)
