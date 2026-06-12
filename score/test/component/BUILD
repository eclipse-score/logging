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
load("@rules_python//python:defs.bzl", "py_library")
load("@score_itf//bazel:py_itf_plugin.bzl", "py_itf_plugin")

pkg_tar(
    name = "dlt_pkg",
    srcs = [
        "@score_itf//third_party/dlt:dlt-receive",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_tar(
    name = "_datarouter_bin_pkg",
    srcs = ["//score/datarouter"],
    package_dir = "opt/datarouter/bin",
    target_compatible_with = ["@platforms//os:linux"],
)

pkg_tar(
    name = "_datarouter_conf_pkg",
    srcs = ["//score/test/component/datarouter:datarouter_test_config_files"],
    package_dir = "opt/datarouter",
    strip_prefix = "/score/test/component/datarouter",
    target_compatible_with = ["@platforms//os:linux"],
)

pkg_tar(
    name = "datarouter_pkg",
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//score/test/component:__subpackages__"],
    deps = [
        ":_datarouter_bin_pkg",
        ":_datarouter_conf_pkg",
    ],
)

py_library(
    name = "logging_plugin_lib",
    srcs = ["logging_plugin.py"],
    imports = ["."],
    target_compatible_with = ["@platforms//os:linux"],
    deps = ["@score_itf//score/itf/plugins/dlt"],
)

py_itf_plugin(
    name = "logging_plugin",
    enabled_plugins = [
        "logging_plugin",
    ],
    py_library = ":logging_plugin_lib",
    tags = ["integration"],
    visibility = ["//score/test/component:__subpackages__"],
)
