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
# ********************************************************************************
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
# ********************************************************************************

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

cc_binary(
    name = "logging_app",
    srcs = ["logging_app.cpp"],
    deps = ["//score/mw/log"],
)

pkg_tar(
    name = "logging_app_bin_pkg",
    srcs = [":logging_app"],
    package_dir = "opt/test_apps/logging_app/bin",
)

pkg_tar(
    name = "logging_app_config_pkg",
    srcs = ["etc/logging.json"],
    package_dir = "opt/test_apps/logging_app/etc",
)

pkg_tar(
    name = "logging_app_filesystem",
    visibility = ["//score/test/component:__subpackages__"],
    deps = [
        ":logging_app_bin_pkg",
        ":logging_app_config_pkg",
    ],
)
