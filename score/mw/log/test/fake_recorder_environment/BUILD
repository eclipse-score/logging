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
    name = "fake_recorder_environment",
    testonly = True,
    srcs = ["fake_recorder_environment.cpp"],
    hdrs = ["fake_recorder_environment.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/log/test/fake_recorder",
        "@googletest//:gtest",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

cc_library(
    name = "auto_register_fake_recorder_env",
    testonly = True,
    srcs = ["register_fake_recorder_environment.cpp"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        ":fake_recorder_environment",
        "@googletest//:gtest",
    ],
    alwayslink = True,  # Ensure static initializer runs
)
