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
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

cc_binary(
    name = "cross_locking_app",
    srcs = ["src/cross_locking_app.cpp"],
    deps = ["//score/mw/log"],
)

pkg_files(
    name = "cross_locking_app_bin_pkg",
    srcs = [":cross_locking_app"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "opt/test_apps/cross_locking_app/bin",
)

pkg_files(
    name = "cross_locking_app_config_pkg",
    srcs = ["etc/logging.json"],
    prefix = "opt/test_apps/cross_locking_app/etc",
)

pkg_filegroup(
    name = "cross_locking_app_filesystem_pkg",
    srcs = [
        ":cross_locking_app_bin_pkg",
        ":cross_locking_app_config_pkg",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_tar(
    name = "cross_locking_app_filesystem",
    srcs = [":cross_locking_app_filesystem_pkg"],
    visibility = ["//score/test/component:__subpackages__"],
)
