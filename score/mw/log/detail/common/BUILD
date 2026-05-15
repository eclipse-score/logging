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
load("@score_baselibs//score/quality/clang_tidy:extra_checks.bzl", "clang_tidy_extra_checks")

clang_tidy_extra_checks(
    name = "clang_tidy_extra_checks",
    extra_features = [
        "spp_code_style_check_header_guards",
        "spp_code_style_check_method_names",
        "spp_code_style_check_readability",
        "spp_code_style_check_type_names",
        "spp_code_style_check_variable_names",
    ],
)

COMPILER_WARNING_FEATURES = [
    "treat_warnings_as_errors",
    "additional_warnings",
    "strict_warnings",
]

cc_library(
    name = "log_entry_deserialize",
    srcs = ["log_entry_deserialize.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test:__subpackages__",
        "@score_logging//score/datarouter:__pkg__",
    ],
    deps = [
        "@score_baselibs//score/mw/log:shared_types",
        "@score_baselibs//score/mw/log/detail:logging_identifier",
        "@score_baselibs//score/static_reflection_with_serialization/visitor",
    ],
)

cc_library(
    name = "helper_functions",
    hdrs = [
        "helper_functions.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
)

cc_library(
    name = "statistics_reporter",
    srcs = [
        "istatistics_reporter.cpp",
        "statistics_reporter.cpp",
    ],
    hdrs = [
        "istatistics_reporter.h",
        "statistics_reporter.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//score/mw/log/detail/data_router:__pkg__",
        "//score/mw/log/detail/file_recorder:__pkg__",
    ],
    deps = [
        "@score_baselibs//score/mw/log:recorder",
    ],
)

cc_library(
    name = "dlt_content_formatting",
    srcs = [
        "dlt_format.cpp",
    ],
    hdrs = [
        "dlt_format.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//score/mw/log/detail/data_router:__pkg__",
        "//score/mw/log/detail/dlt_trace:__pkg__",
        "//score/mw/log/detail/file_recorder:__pkg__",
    ],
    deps = [
        ":helper_functions",
        "@score_baselibs//score/bitmanipulation",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/memory:string_literal",
        "@score_baselibs//score/mw/log:shared_types",
        "@score_baselibs//score/mw/log/detail:dlt_argument_counter",
        "@score_baselibs//score/mw/log/detail:integer_representation",
        "@score_baselibs//score/mw/log/detail:log_data_types",
    ],
)

cc_test(
    name = "dlt_format_test",
    srcs = [
        "dlt_format_test.cpp",
    ],
    data = [
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        ":dlt_content_formatting",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "log_entry_deserialize_test",
    srcs = [
        "log_entry_deserialize_test.cpp",
    ],
    data = [
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        ":log_entry_deserialize",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "statistics_reporter_test",
    srcs = [
        "statistics_reporter_test.cpp",
    ],
    data = [
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        ":statistics_reporter",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:recorder_mock",
    ],
)

cc_test(
    name = "helper_functions_test",
    srcs = [
        "helper_functions_test.cpp",
    ],
    data = [
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        ":helper_functions",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "runtime_test",
    srcs = [
        ":runtime_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    local_defines = select({
        "//score/mw/log/detail/flags:config_KConsole_Logging": ["KCONSOLE_LOGGING"],
        "//conditions:default": [],
    }),
    tags = ["unit"],
    deps = [
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console",
        "@score_baselibs//score/mw/log:recorder_mock",
    ] + select({
        "//score/mw/log/detail/flags:config_KConsole_Logging": [
            "@score_baselibs//score/mw/log/detail/text_recorder",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "log_stream_test",
    srcs = [
        "@score_baselibs//score/mw/log:log_stream_test.cpp",
    ],
    copts = ["-Wno-deprecated-declarations"],  # src from @score_baselibs uses deprecated APIs
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = [
        "no-clang-tidy",  # src from @score_baselibs, cannot suppress deprecations in source
        "unit",
    ],
    deps = [
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console",
        "@score_baselibs//score/mw/log:recorder_mock",
        "@score_baselibs//score/mw/log/test/my_custom_lib:my_custom_type_mw_log",
    ],
)

cc_test(
    name = "log_stream_factory_test",
    srcs = [
        "@score_baselibs//score/mw/log:log_stream_factory_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    deps = [
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log:console",
        "@score_baselibs//score/mw/log:recorder_mock",
        "@score_baselibs//score/mw/log/detail:thread_local_guard",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_tests",
    cc_unit_tests = [
        ":dlt_format_test",
        ":log_entry_deserialize_test",
        ":statistics_reporter_test",
        ":helper_functions_test",
        ":runtime_test",
        ":log_stream_test",
        ":log_stream_factory_test",
    ],
    visibility = [
        "@score_baselibs//score/mw/log/detail:__pkg__",
    ],
)
