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
#
# BUILD file for score/evaluation
#
# Each cc_library target corresponds to one evaluation scenario file.
# The cc_binary target "evaluation_all" links every library and provides
# a single compilable artefact that the code-review tool can analyse.
#
# NOTE: These targets intentionally contain anti-patterns and bad practices.
#       They are NOT suitable for production use and exist solely for
#       evaluating the capability of automated code-review tools.
# *******************************************************************************

load("@rules_cc//cc:defs.bzl", "cc_binary")

alias(
    name = "solid_violations",
    actual = "//score/evaluation/solid:solid_violations",
)

alias(
    name = "memory_issues",
    actual = "//score/evaluation/memory:memory_issues",
)

alias(
    name = "non_idiomatic_cpp",
    actual = "//score/evaluation/idiomatic:non_idiomatic_cpp",
)

alias(
    name = "modern_cpp_syntax",
    actual = "//score/evaluation/modern_cpp:modern_cpp_syntax",
)

alias(
    name = "design_patterns",
    actual = "//score/evaluation/patterns:design_patterns",
)

alias(
    name = "code_optimization",
    actual = "//score/evaluation/optimization:code_optimization",
)

alias(
    name = "safe_cpp",
    actual = "//score/evaluation/safety:safe_cpp",
)

alias(
    name = "security_issues",
    actual = "//score/evaluation/security:security_issues",
)

alias(
    name = "design_faults",
    actual = "//score/evaluation/design:design_faults",
)

alias(
    name = "concurrency_issues",
    actual = "//score/evaluation/concurrency:concurrency_issues",
)

alias(
    name = "exception_error_handling",
    actual = "//score/evaluation/exception_handling:exception_error_handling",
)

alias(
    name = "unit_test_quality",
    actual = "//score/evaluation/unit_test_quality:unit_test_quality",
)

alias(
    name = "template_metaprogramming_issues",
    actual = "//score/evaluation/template_metaprogramming:template_metaprogramming_issues",
)

alias(
    name = "unit_test_quality_test",
    actual = "//score/evaluation/unit_test_quality:unit_test_quality_test",
)

# ---------------------------------------------------------------------------
# Aggregate binary – compiles all evaluation scenarios together.
# ---------------------------------------------------------------------------
cc_binary(
    name = "evaluation_all",
    srcs = ["evaluation_main.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//score/evaluation/concurrency:concurrency_issues",
        "//score/evaluation/design:design_faults",
        "//score/evaluation/exception_handling:exception_error_handling",
        "//score/evaluation/idiomatic:non_idiomatic_cpp",
        "//score/evaluation/memory:memory_issues",
        "//score/evaluation/modern_cpp:modern_cpp_syntax",
        "//score/evaluation/optimization:code_optimization",
        "//score/evaluation/patterns:design_patterns",
        "//score/evaluation/security:security_issues",
        "//score/evaluation/safety:safe_cpp",
        "//score/evaluation/solid:solid_violations",
        "//score/evaluation/template_metaprogramming:template_metaprogramming_issues",
        "//score/evaluation/unit_test_quality:unit_test_quality",
    ],
)
