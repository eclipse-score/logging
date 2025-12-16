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

load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "logging_error",
    srcs = [
        "error.cpp",
    ],
    hdrs = [
        "error.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/datarouter:__subpackages__"],
    deps = [
        "@score_baselibs//score/memory:string_literal",
        "@score_baselibs//score/result",
    ],
)
