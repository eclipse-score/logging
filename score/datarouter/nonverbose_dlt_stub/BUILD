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
        name = "nonverbose_dlt_handler_stub" + name,
        testonly = test_only,
        hdrs = [
            "stub_nonverbose_dlt.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        strip_include_prefix = ".",
        visibility = [
            "//score/datarouter:__subpackages__",
        ],
        deps = [
            "//score/datarouter:log",
            "//score/datarouter:logparser",
            "@score_baselibs//score/mw/log/configuration:nvconfig",
        ] + dep,
    )
    for name, dep, test_only in [
        (
            "",
            [
                "//score/datarouter:dlt_log_channel_lib",
            ],
            False,
        ),
        (
            "_testing",
            [
                "//score/datarouter:dlt_log_channel_lib_testing",
            ],
            True,
        ),
    ]
]
