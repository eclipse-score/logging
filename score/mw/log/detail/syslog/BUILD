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

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//:score/mw/common_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "syslog_backend",
    srcs = [
        "syslog_backend.cpp",
    ],
    hdrs = [
        "syslog_backend.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = [
        "//score/mw/log:__subpackages__",
        "@score_baselibs//score/mw/log:__subpackages__",
    ],
    deps = [
        "@score_baselibs//score/mw/log/detail:backend_interface",
        "@score_baselibs//score/mw/log/detail:circular_allocator",
        "@score_baselibs//score/mw/log/detail:initialization_reporter",
        "@score_baselibs//score/os:syslog",
    ],
)

cc_library(
    name = "syslog",
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = [
        "//score/mw/log:__subpackages__",
        "@score_baselibs//score/mw/log:__subpackages__",
    ],
    deps = [
        ":syslog_backend",
        ":syslog_recorder_factory",
    ],
)

cc_library(
    name = "syslog_recorder_factory",
    srcs = [
        "syslog_recorder_factory.cpp",
    ],
    hdrs = [
        "syslog_recorder_factory.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    target_compatible_with = ["@platforms//os:linux"],
    visibility = [
        "//score/mw/log:__subpackages__",
        "@score_baselibs//score/mw/log:__subpackages__",
    ],
    deps = [
        ":syslog_backend",
        "@score_baselibs//score/mw/log/detail:log_recorder_factory",
        "@score_baselibs//score/mw/log/detail/text_recorder",
    ],
)

cc_test(
    name = "syslog_recorder_factory_test",
    srcs = [
        "syslog_recorder_factory_test.cpp",
    ],
    features = [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        ":syslog_recorder_factory",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/futurecpp:futurecpp_test_support",
        "@score_baselibs//score/mw/log/configuration",
    ],
)

cc_test(
    name = "syslog_backend_test",
    srcs = [
        "syslog_backend_test.cpp",
    ],
    features = [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    target_compatible_with = ["@platforms//os:linux"],
    deps = [
        ":syslog",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/futurecpp:futurecpp_test_support",
        "@score_baselibs//score/mw/log/configuration",
        "@score_baselibs//score/os/mocklib:syslog_mock",
    ],
)
