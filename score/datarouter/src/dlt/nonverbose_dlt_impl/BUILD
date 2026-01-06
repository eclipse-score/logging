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

load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")
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

[
    cc_library(
        name = "nonverbose_dlt_handler" + name,
        testonly = test_only,
        srcs = [
            "nonverbose_dlt.cpp",
        ],
        hdrs = [
            "nonverbose_dlt.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        strip_include_prefix = ".",
        visibility = [
            "@score_logging//score/datarouter:__subpackages__",
        ],
        deps = [
            "@score_baselibs//score/mw/log/configuration:nvconfig",
            "@score_logging//score/datarouter:log",
            "@score_logging//score/datarouter:logparser",
        ] + dep,
    )
    for name, dep, test_only in [
        (
            "",
            [
                "@score_logging//score/datarouter:dlt_log_channel_lib",
            ],
            False,
        ),
        (
            "_testing",
            [
                "@score_logging//score/datarouter:dlt_log_channel_lib_testing",
            ],
            True,
        ),
    ]
]
