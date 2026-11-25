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

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")

cc_binary(
    name = "fault_app",
    srcs = ["src/fault_app.cpp"],
    deps = ["//score/mw/log"],
)

pkg_files(
    name = "fault_app_bin_pkg",
    srcs = [":fault_app"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "opt/test_apps/fault_app/bin",
)

pkg_files(
    name = "fault_app_config_pkg",
    srcs = ["etc/logging.json"],
    prefix = "opt/test_apps/fault_app/etc",
)

pkg_filegroup(
    name = "fault_app_filesystem_pkg",
    srcs = [
        ":fault_app_bin_pkg",
        ":fault_app_config_pkg",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)
