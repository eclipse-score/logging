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

cc_library(
    name = "common",
    srcs = ["common.cpp"],
    hdrs = ["common.h"],
    features = [
        "treat_warnings_as_errors",
        "additional_warnings",
        "strict_warnings",
    ],
    tags = ["FFI"],
    visibility = ["@score_baselibs//score/mw/log/detail:__subpackages__"],
    deps = [
        "//score/mw/log/detail/wait_free_producer_queue:alternating_control_block",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/os/utils:high_resolution_steady_clock",
    ],
)

cc_library(
    name = "writer",
    srcs = [
        "shared_memory_writer.cpp",
        "writer_factory.cpp",
    ],
    hdrs = [
        "shared_memory_writer.h",
        "writer_factory.h",
    ],
    features = [
        "treat_warnings_as_errors",
        "additional_warnings",
        "strict_warnings",
    ],
    tags = ["FFI"],
    visibility = [
        "//score/mw/log/detail/data_router:__subpackages__",
        "//score/mw/log/legacy_non_verbose_api:__subpackages__",
    ],
    deps = [
        ":common",
        "//score/mw/log/detail/wait_free_producer_queue:alternating_proxy_reader",
        "//score/mw/log/detail/wait_free_producer_queue:alternating_writer",
        "@score_baselibs//score/os:fcntl",
        "@score_baselibs//score/os:mman",
        "@score_baselibs//score/os:pthread",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/os:unistd",
    ],
)

cc_library(
    name = "reader",
    srcs = [
        "reader_factory.cpp",
        "reader_factory_impl.cpp",
        "shared_memory_reader.cpp",
    ],
    hdrs = [
        "i_shared_memory_reader.h",
        "reader_factory.h",
        "reader_factory_impl.h",
        "shared_memory_reader.h",
    ],
    features = [
        "treat_warnings_as_errors",
        "additional_warnings",
        "strict_warnings",
    ],
    tags = ["FFI"],
    visibility = [
        "//score/datarouter:__subpackages__",
        "//score/mw/log/detail/data_router:__subpackages__",
        "//score/mw/log/legacy_non_verbose_api:__subpackages__",
    ],
    deps = [
        ":common",
        "//score/mw/log/detail/wait_free_producer_queue:read_only_reader",
        "@score_baselibs//score/os:mman",
        "@score_baselibs//score/os:stat",
    ],
)

cc_library(
    name = "reader_factory_mock",
    testonly = True,
    srcs = [
        "reader_factory_mock.h",
    ],
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = ["@googletest//:gtest"],
)

cc_library(
    name = "shared_memory_reader_mock",
    testonly = True,
    srcs = [
        "shared_memory_reader_mock.h",
    ],
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = ["@googletest//:gtest"],
)

cc_test(
    name = "unit_test",
    srcs = [
        "common_test.cpp",
        "reader_factory_test.cpp",
        "shared_memory_reader_test.cpp",
        "shared_memory_writer_test.cpp",
        "writer_factory_test.cpp",
    ],
    features = [
        "aborts_upon_exception",
        "treat_warnings_as_errors",
        "additional_warnings",
        "strict_warnings",
    ],
    linkopts = select({
        "@platforms//os:linux": ["-latomic"],  # needed for spp-host-clang
        "//conditions:default": [],
    }),
    tags = ["unit"],
    visibility = ["//score/mw/log/detail/data_router:__pkg__"],
    deps = [
        ":reader",
        ":writer",
        "//score/mw/log/test/console_logging_environment",
        "//third_party/libatomic",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:fcntl_mock",
        "@score_baselibs//score/os/mocklib:mman_mock",
        "@score_baselibs//score/os/mocklib:stat_mock",
        "@score_baselibs//score/os/mocklib:stdlib_mock",
        "@score_baselibs//score/os/mocklib:unistd_mock",
    ],
)

cc_unit_test_suites_for_host_and_qnx(
    name = "unit_tests",
    cc_unit_tests = [
        ":unit_test",
    ],
    visibility = ["//score/mw/log/detail/data_router:__pkg__"],
)
