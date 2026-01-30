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

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_rust//rust:defs.bzl", "rust_binary", "rust_library", "rust_test")

config_setting(
    name = "x86_64-linux",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
)

config_setting(
    name = "arm64-qnx",
    constraint_values = [
        "@platforms//cpu:arm64",
        "@platforms//os:qnx",
    ],
)

config_setting(
    name = "x86_64-qnx",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:qnx",
    ],
)

cc_library(
    name = "adapter",
    srcs = ["src/adapter.cpp"],
    # C++/Rust interface objects must share layout.
    defines = select({
        ":x86_64-linux": ["x86_64_linux"],
        ":arm64-qnx": ["arm64_qnx"],
        ":x86_64-qnx": ["x86_64_qnx"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:private"],
    deps = [
        "//score/mw/log/detail/common:recorder_factory",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

rust_library(
    name = "score_log_bridge",
    srcs = glob(["src/**/*.rs"]),
    # C++/Rust interface objects must share layout.
    crate_features = select({
        ":x86_64-linux": ["x86_64_linux"],
        ":arm64-qnx": ["arm64_qnx"],
        ":x86_64-qnx": ["x86_64_qnx"],
        "//conditions:default": [],
    }),
    edition = "2021",
    visibility = ["//visibility:public"],
    deps = [
        ":adapter",
        "@score_baselibs_rust//src/log/score_log",
    ],
)

RUSTC_FLAGS = select({
    "@platforms//os:qnx": [
        "-Clink-arg=-lc++",
        "-Clink-arg=-lm",
    ],
    "//conditions:default": [
        "-Clink-arg=-lstdc++",
        "-Clink-arg=-lm",
        "-Clink-arg=-lc",
    ],
})

rust_test(
    name = "tests",
    crate = "score_log_bridge",
    crate_features = select({
        ":x86_64-linux": ["x86_64_linux"],
        ":arm64-qnx": ["arm64_qnx"],
        "//conditions:default": [],
    }),
    edition = "2021",
    rustc_flags = RUSTC_FLAGS,
    tags = [
        "unit_tests",
        "ut",
    ],
    deps = [
        "@score_crates//:libc",
        "@score_crates//:tempfile",
    ],
)

rust_binary(
    name = "example",
    srcs = ["examples/main.rs"],
    data = [
        "examples/config/logging.json",
    ],
    edition = "2021",
    rustc_flags = RUSTC_FLAGS,
    visibility = ["//visibility:public"],
    deps = [
        ":score_log_bridge",
        "@score_baselibs_rust//src/log/score_log",
    ],
)
