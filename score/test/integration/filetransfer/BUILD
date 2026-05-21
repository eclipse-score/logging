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
load("//score/test/integration:defs.bzl", "py_logging_itf_test")

cc_binary(
    name = "filetransfer_app",
    srcs = ["filetransfer_app.cpp"],
    deps = [
        "//score/datarouter/dlt_filetransfer_trigger_lib",
        "//score/mw/log",
    ],
)

pkg_tar(
    name = "filetransfer_bin_pkg",
    srcs = [":filetransfer_app"],
    package_dir = "opt/test_apps/filetransfer/bin",
)

pkg_tar(
    name = "filetransfer_config_pkg",
    srcs = ["etc/logging.json"],
    package_dir = "opt/test_apps/filetransfer/etc",
)

pkg_tar(
    name = "filetransfer_filesystem",
    deps = [
        ":filetransfer_bin_pkg",
        ":filetransfer_config_pkg",
    ],
)

py_logging_itf_test(
    name = "test_filetransfer",
    srcs = ["test_filetransfer.py"],
    filesystem = ":filetransfer_filesystem",
    deps = ["//score/test/integration:dlt_parser"],
)
