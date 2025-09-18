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

load("@rules_rust//cargo:defs.bzl", "cargo_build_script")
load("@rules_rust//rust:defs.bzl", "rust_binary", "rust_library")

cc_library(
    name = "libmw_log_ffi",
    srcs = ["src/mw_log_ffi.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "@score-baselibs//score/mw/log",
    ],
)

rust_library(
    name = "mw_log_subscriber",
    srcs = glob(["src/**/*.rs"]),
    crate_name = "mw_log_subscriber",
    edition = "2021",
    visibility = ["//visibility:public"],
    deps = [
        ":libmw_log_ffi",
        "@score-baselibs//score/mw/log/rust:mw_log",
    ],
)

rust_binary(
    name = "example",
    srcs = ["examples/main.rs"],
    data = [
        "examples/config/logging.json",
    ],
    edition = "2021",
    visibility = ["//visibility:public"],
    deps = [
        ":mw_log_subscriber",
        "@score-baselibs//score/mw/log/rust:mw_log",
    ],
)
