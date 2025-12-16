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

[
    cc_library(
        name = "config_session_factory" + name,
        testonly = test_only,
        hdrs = [
            "config_session_factory.hpp",
        ],
        features = COMPILER_WARNING_FEATURES,
        strip_include_prefix = ".",
        visibility = [
            "//score/datarouter:__subpackages__",
        ],
        deps = deps,
    )
    for name, test_only, deps in [
        (
            "",
            False,
            [
                "//score/datarouter/dynamic_configuration/i_session",
            ],
        ),
        (
            "_testing",
            True,
            [],
        ),
    ]
]
