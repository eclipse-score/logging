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

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

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
            "@score_baselibs//score/mw/log",
            "@score_baselibs//score/mw/log/configuration:nvconfig",
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
