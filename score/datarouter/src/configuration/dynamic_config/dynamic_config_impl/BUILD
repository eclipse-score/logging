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
        name = "dynamic_config_session" + name,
        testonly = test_only,
        hdrs = [
            "dynamic_config_session.h",
            "dynamic_config_session_factory.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        strip_include_prefix = ".",
        visibility = visibility,
        deps = deps + [
            "//score/datarouter/src/configuration/dynamic_config:i_session",
        ],
    )
    for name, test_only, visibility, deps in [
        (
            "",
            False,
            [
                "@score_logging//score/datarouter:__pkg__",
                "@score_logging//score/datarouter:__subpackages__",
            ],
            [
                "@score_logging//score/datarouter:unixdomain_server",
                "//score/datarouter/src/configuration/dynamic_config:config_session_factory",
            ],
        ),
        (
            "_testing",
            True,
            [
                "@score_logging//score/datarouter:__pkg__",
                "//score/datarouter/test:__subpackages__",
            ],
            [
                "@score_logging//score/datarouter:unixdomain_mock",
                "//score/datarouter/src/configuration/dynamic_config:config_session_factory_testing",
            ],
        ),
    ]
]
