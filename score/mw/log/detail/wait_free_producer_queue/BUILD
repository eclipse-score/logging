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

load("@score_baselibs//:bazel/unit_tests.bzl", "cc_unit_test_suites_for_host_and_qnx")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "read_only_reader",
    srcs = [
        "alternating_reader.cpp",
        "linear_reader.cpp",
    ],
    hdrs = [
        "alternating_reader.h",
        "linear_reader.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/mw/log/detail/data_router/shared_memory:__subpackages__"],
    deps = [
        "alternating_control_block",
    ],
)

cc_library(
    name = "alternating_writer",
    srcs = [
        "wait_free_alternating_writer.cpp",
        "wait_free_linear_writer.cpp",
    ],
    hdrs = [
        "wait_free_alternating_writer.h",
        "wait_free_linear_writer.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/mw/log/detail/data_router/shared_memory:__subpackages__"],
    deps = [
        "alternating_control_block",
    ],
)

cc_library(
    name = "alternating_control_block",
    srcs = [
        "alternating_control_block.cpp",
        "linear_control_block.cpp",
    ],
    hdrs = [
        "alternating_control_block.h",
        "linear_control_block.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/mw/log/detail/data_router/shared_memory:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "alternating_proxy_reader",
    srcs = [
        "alternating_reader_proxy.cpp",
    ],
    hdrs = [
        "alternating_reader_proxy.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//score/mw/log/detail/data_router/shared_memory:__subpackages__"],
    deps = [
        "alternating_control_block",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_test(
    name = "unit_test",
    srcs = [
        "alternating_control_block_test.cpp",
        "linear_control_block_test.cpp",
        "linear_reader_test.cpp",
        "wait_free_alternating_writer_test.cpp",
        "wait_free_linear_writer_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    linkopts = select({
        "@platforms//os:linux": ["-latomic"],  # needed for spp-host-clang
        "//conditions:default": [],
    }),
    tags = ["unit"],
    visibility = ["@score_baselibs//score/mw/log/detail:__pkg__"],
    deps = [
        "alternating_proxy_reader",
        "alternating_writer",
        "read_only_reader",
        "//score/mw/log/test/console_logging_environment",
        "//third_party/libatomic",
        "@googletest//:gtest_main",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_tests",
    cc_unit_tests = [
        ":unit_test",
    ],
    visibility = ["@score_baselibs//score/mw/log/detail:__pkg__"],
)
