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

cc_binary(
    name = "malloc-profiler",
    srcs = ["malloc-profiler.c"],
    linkshared = True,
    local_defines = ["CSV_TRACE"] + select({
        "@platforms//os:qnx": ["_QNX_SOURCE"],
        "//conditions:default": [],
    }),
    visibility = ["//score/test/component:__subpackages__"],
)
