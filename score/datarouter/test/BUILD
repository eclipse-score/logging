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
load("@score_baselibs//third_party/itf:py_unittest_qnx_test.bzl", "py_unittest_qnx_test")

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

## ---------------------------------------------------------------------------
## Unit tests
## ---------------------------------------------------------------------------

test_suite(
    name = "unit_tests",
    tests = [
        "//score/datarouter/test/ut:unit_tests",
    ],
    visibility = ["@score_logging//score/datarouter:__pkg__"],
)

py_unittest_qnx_test(
    name = "unit_tests_qnx",
    test_suites = [
        "//score/datarouter/test/ut:unit_tests_qnx",
    ],
    visibility = ["//visibility:public"],  # platform_only
)
