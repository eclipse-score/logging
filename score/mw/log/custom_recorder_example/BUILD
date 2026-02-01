load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

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

cc_library(
    name = "custom_recorder_example",
    srcs = [
        "custom_recorder_factory_impl.cpp",
        "custom_recorder_impl.cpp",
    ],
    hdrs = [
        "custom_recorder.h",
        "custom_recorder_factory_impl.h",
        "custom_recorder_impl.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log:recorder",
        "@score_baselibs//score/mw/log/detail:log_recorder_factory",
    ],
)

cc_binary(
    name = "test",
    srcs = [
        "main.cpp",
    ],
    deps = [
        "//score/mw/log/detail/common:recorder_factory",
        "@score_baselibs//score/mw/log:frontend",
    ],
)
