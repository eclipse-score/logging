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

load("@rules_cc//cc:defs.bzl", "cc_library")

COMPILER_WARNING_FEATURES = [
    "treat_warnings_as_errors",
    "additional_warnings",
    "strict_warnings",
]

cc_library(
    name = "log",
    deprecation = "Usage of target '@score_logging//score/mw/log' is deprecated and will be removed in the v0.8 release. Please use '@score_baselibs//score/mw/log' instead.",
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/log/backend:file",
        "//score/mw/log/backend:remote",
        "@score_baselibs//score/mw/log:console",
        "@score_baselibs//score/mw/log:frontend",
    ] + select({
        "@platforms//os:qnx": ["//score/mw/log/backend:slog"],
        "//conditions:default": [],
    }),
)
