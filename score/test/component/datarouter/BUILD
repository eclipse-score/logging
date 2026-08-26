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

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("//score/test/component:defs.bzl", "py_logging_itf_test")

# =============================================================================
# filetransfer app
# =============================================================================

cc_binary(
    name = "filetransfer_app",
    srcs = ["filetransfer_app.cpp"],
    visibility = ["//score/test/component:__subpackages__"],
    deps = [
        "//score/datarouter/dlt_filetransfer_trigger_lib",
        "//score/mw/log",
    ],
)

pkg_files(
    name = "filetransfer_bin_pkg",
    srcs = [":filetransfer_app"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "opt/test_apps/filetransfer/bin",
)

pkg_files(
    name = "filetransfer_config_pkg",
    srcs = ["filetransfer_etc/logging.json"],
    prefix = "opt/test_apps/filetransfer/etc",
)

pkg_filegroup(
    name = "filetransfer_filesystem_pkg",
    srcs = [
        ":filetransfer_bin_pkg",
        ":filetransfer_config_pkg",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_tar(
    name = "filetransfer_filesystem",
    srcs = [":filetransfer_filesystem_pkg"],
)

py_logging_itf_test(
    name = "test_filetransfer",
    srcs = ["test_filetransfer.py"],
    filesystem = ":filetransfer_filesystem",
    filesystem_pkg = ":filetransfer_filesystem_pkg",
)

# =============================================================================
# datarouter filters test
# =============================================================================

filegroup(
    name = "datarouter_test_config_files",
    srcs = [
        "etc/log-channels.json",
        "etc/logging.json",
        "etc/raw-channels.json",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

# Packaged for inclusion in QNX component test images via qnx_ifs `srcs`.
pkg_files(
    name = "datarouter_bin_pkg",
    srcs = ["//score/datarouter"],
    attributes = pkg_attributes(mode = "0777"),
    prefix = "usr/bin/datarouter",
    visibility = ["//score/test/component:__subpackages__"],
)

# Packaged for inclusion in QNX component test images via qnx_ifs `srcs`.
pkg_files(
    name = "datarouter_config_pkg",
    srcs = [":datarouter_test_config_files"],
    prefix = "usr/bin/datarouter/etc",
    visibility = ["//score/test/component:__subpackages__"],
)

py_logging_itf_test(
    name = "test_datarouter_filters",
    srcs = ["test_datarouter_filters.py"],
    filesystem = "//score/test/component/filters_app:filtertest_filesystem",
    filesystem_pkg = "//score/test/component/filters_app:filtertest_filesystem_pkg",
    deps = ["@score_itf//score/itf/plugins/dlt"],
)
