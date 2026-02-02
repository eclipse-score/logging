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

# Synchronized utility BUILD file

FEAT_COMPILER_WARNINGS_AS_ERRORS = [
    "treat_warnings_as_errors",
    "strict_warnings",
    "additional_warnings",
]

cc_library(
    name = "synchronized_utility",
    hdrs = ["synchronized.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "synchronized_test",
    srcs = ["synchronized_test.cpp"],
    copts = ["-std=c++17"],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    deps = [
        ":synchronized_utility",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
