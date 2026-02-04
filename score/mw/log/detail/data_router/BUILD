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
    name = "data_router_backend",
    srcs = [
        "data_router_backend.cpp",
        "data_router_message_client.cpp",
        "data_router_message_client_backend.cpp",
        "data_router_message_client_factory.cpp",
        "data_router_message_client_factory_impl.cpp",
        "data_router_message_client_identifiers.cpp",
        "data_router_message_client_impl.cpp",
        "data_router_message_client_utils.cpp",
        "data_router_recorder.cpp",
        "message_passing_factory.cpp",
        "message_passing_factory_impl.cpp",
    ],
    hdrs = [
        "data_router_backend.h",
        "data_router_message_client.h",
        "data_router_message_client_backend.h",
        "data_router_message_client_factory.h",
        "data_router_message_client_factory_impl.h",
        "data_router_message_client_identifiers.h",
        "data_router_message_client_impl.h",
        "data_router_message_client_utils.h",
        "data_router_recorder.h",
        "message_passing_factory.h",
        "message_passing_factory_impl.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//score/datarouter/test:__subpackages__",
        "//score/mw/log/legacy_non_verbose_api:__pkg__",
        "@score_baselibs//score/mw/log:__pkg__",
        "@score_baselibs//score/mw/log/detail:__pkg__",
    ],
    deps = [
        ":message_passing_interface",
        "//score/mw/log/detail/common:dlt_content_formatting",
        "//score/mw/log/detail/common:statistics_reporter",
        "//score/mw/log/detail/data_router/shared_memory:writer",
        "//score/mw/log/detail/utils/signal_handling",
        "//score/mw/log/legacy_non_verbose_api",
        "@score_baselibs//score/concurrency",
        "@score_baselibs//score/concurrency:thread_pool",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/memory/shared",  # Preperation for full rollout of (Ticket-105741), build cyclic dependency is broken (runtime, in next commit)
        "@score_baselibs//score/mw/log:recorder",
        "@score_baselibs//score/mw/log/configuration",
        "@score_baselibs//score/mw/log/detail:backend_interface",
        "@score_baselibs//score/mw/log/detail:circular_allocator",
        "@score_baselibs//score/mw/log/detail:dlt_argument_counter",
        "@score_baselibs//score/mw/log/detail:initialization_reporter",
        "@score_baselibs//score/mw/log/detail:log_data_types",
        "@score_baselibs//score/mw/log/detail:types_and_errors",
        "@score_baselibs//score/mw/log/detail/wait_free_stack",
        "@score_baselibs//score/os:pthread",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os/utils:signal",
        "@score_communication//score/message_passing",
    ],
)

cc_library(
    name = "remote_dlt_recorder_factory",
    srcs = [
        "remote_dlt_recorder_factory.cpp",
    ],
    hdrs = [
        "remote_dlt_recorder_factory.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//score/mw/log/detail/common:__pkg__",
        "@score_baselibs//score/mw/log/detail:__pkg__",
    ],
    deps = [
        ":data_router_backend",
        "@score_baselibs//score/mw/log:recorder_interface",
        "@score_baselibs//score/mw/log/configuration",
        "@score_baselibs//score/mw/log/detail:log_recorder_factory",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_test(
    name = "unit_test",
    srcs = [
        "data_router_backend_test.cpp",
        "data_router_message_client_factory_mock.h",
        "data_router_message_client_factory_test.cpp",
        "data_router_message_client_mock.h",
        "data_router_message_client_test.cpp",
        "data_router_messages_test.cpp",
        "data_router_recorder_test.cpp",
        "message_passing_factory_mock.h",
        "message_passing_factory_test.cpp",
        "remote_dlt_recorder_factory_test.cpp",
    ],
    features = COMPILER_WARNING_FEATURES + [
        "aborts_upon_exception",
    ],
    tags = ["unit"],
    visibility = ["@score_baselibs//score/mw/log/detail:__pkg__"],
    deps = [
        ":data_router_backend",
        ":message_passing_interface",
        "//score/mw/log/detail/common:recorder_factory",
        "//score/mw/log/detail/data_router/shared_memory:reader",
        "//score/mw/log/test/console_logging_environment",
        "@googletest//:gtest_main",
        "@score_baselibs//score/language/futurecpp:futurecpp_test_support",
        "@score_baselibs//score/mw/log/configuration:target_config_reader_mock",
        "@score_baselibs//score/mw/log/detail:backend_mock",
        "@score_baselibs//score/os/mocklib:fcntl_mock",
        "@score_baselibs//score/os/mocklib:mman_mock",
        "@score_baselibs//score/os/mocklib:pthread_mock",
        "@score_baselibs//score/os/mocklib:stat_mock",
        "@score_baselibs//score/os/mocklib:stdlib_mock",
        "@score_baselibs//score/os/mocklib:unistd_mock",
        "@score_baselibs//score/os/utils/mocklib:signal_mock",
        "@score_communication//score/message_passing:mock",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_tests",
    cc_unit_tests = [
        ":unit_test",
    ],
    test_suites_from_sub_packages = ["//score/mw/log/detail/data_router/shared_memory:unit_tests"],
    visibility = ["@score_baselibs//score/mw/log/detail:__pkg__"],
)

cc_library(
    name = "message_passing_interface",
    srcs = [
        "data_router_messages.cpp",
    ],
    hdrs = [
        "data_router_messages.h",
        "message_passing_config.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = [
        "//score/datarouter/test:__subpackages__",
        "//score/mw/log/detail/slog:__pkg__",
        "@score_logging//score/datarouter:__pkg__",
    ],
    deps = [
        "@score_baselibs//score/mw/log/detail:logging_identifier",
    ],
)
