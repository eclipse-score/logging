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

load("@rules_python//python:defs.bzl", "py_library")
load("@score_itf//bazel:py_itf_plugin.bzl", "py_itf_plugin")

py_library(
    name = "logging_plugin_lib",
    srcs = ["logging_plugin.py"],
    imports = ["."],
    tags = ["integration"],
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
