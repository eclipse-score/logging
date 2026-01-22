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

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_rust//rust:defs.bzl", "rust_static_library")

rust_static_library(
    name = "ffi",
    srcs = ["ffi.rs"],
    edition = "2021",
    visibility = ["//visibility:private"],
    deps = [
        "//score/mw/log/rust/score_log_bridge",
    ],
)

cc_library(
    name = "score_log_bridge_cpp_init",
    srcs = ["score_log_bridge_init.cpp"],
    hdrs = ["score_log_bridge_init.h"],
    include_prefix = "score/mw/log/rust",
    visibility = ["//visibility:public"],
    deps = [
        ":ffi",
        # Link dependency required by `:ffi`.
        "//score/mw/log/detail/common:recorder_factory",
    ],
)

# Example consists of Rust library, C++ library, C++ application.

rust_static_library(
    name = "example_lib",
    srcs = ["examples/example_lib.rs"],
    edition = "2021",
    visibility = ["//visibility:private"],
    deps = [
        "//score/mw/log/rust/score_log_bridge",
        "@score_baselibs_rust//src/log/score_log",
    ],
)

cc_binary(
    name = "example",
    srcs = ["examples/main.cpp"],
    linkopts = select({
        "@platforms//os:qnx": [
            "-lsocket",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":example_lib",
        ":score_log_bridge_cpp_init",
    ],
)
