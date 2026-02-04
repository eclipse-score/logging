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

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@score_baselibs//:bazel/unit_tests.bzl", "cc_unit_test_suites_for_host_and_qnx")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "legacy_non_verbose_api",
    srcs = ["tracing.cpp"],
    hdrs = ["tracing.h"],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//platform/aas/ara/log:__subpackages__",
        "//score/mw/log:__subpackages__",
        "@score_baselibs//score/mw/log:__subpackages__",
        "@score_logging//score/datarouter:__pkg__",
    ],
    deps = [
        "//score/mw/log/detail/data_router/shared_memory:writer",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log:frontend",
        "@score_baselibs//score/mw/log/configuration",
        "@score_baselibs//score/mw/log/configuration:nvconfig",
        "@score_baselibs//score/mw/log/configuration:nvconfigfactory",
        "@score_baselibs//score/mw/log/detail/wait_free_stack",
        "@score_baselibs//score/static_reflection_with_serialization/serialization",
        "@score_logging//score/datarouter:libtracing_legacy_public_headers",
    ],
)

filegroup(
    name = "unit_test_data",
    testonly = True,
    srcs = [
        "test/error-content-json-class-id.json",
        "test/test-class-id.json",
    ],
)

cc_test(
    name = "unit_test",
    srcs = ["tracing_test.cpp"],
    data = [":unit_test_data"],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        "//score/datarouter/dlt_filetransfer_trigger_lib:filetransfer_message_types",
        "//score/mw/log/detail/data_router:data_router_backend",
        "//score/mw/log/detail/data_router/shared_memory:reader",
        "//score/mw/log/test/console_logging_environment",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/futurecpp:futurecpp_test_support",
        "@score_baselibs//score/mw/log:frontend",
        "@score_baselibs//score/os/mocklib:pthread_mock",
        "@score_baselibs//score/os/mocklib:stat_mock",
        "@score_baselibs//score/os/mocklib:stdlib_mock",
        "@score_baselibs//score/os/mocklib:unistd_mock",
        "@score_baselibs//score/os/utils/mocklib:signal_mock",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_tests",
    cc_unit_tests = [":unit_test"],
    visibility = ["@score_baselibs//score/mw/log:__pkg__"],
)
