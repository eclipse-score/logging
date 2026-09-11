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

load(
    "@score_tooling//bazel/rules/rules_score:rules_score.bzl",
    "assumptions_of_use",
)

# aou_requirements.rst is score_baselibs' mw::log *frontend* AoUs (POC: local_path_override
# to ../score_baselibs in //MODULE.bazel). mw::log's frontend lives in score_baselibs while
# this backend lives here; the backend doesn't mitigate these itself, it just forwards them
# onward to its own integrators alongside aou.trlc's backend-specific AoU.
assumptions_of_use(
    name = "assumptions_of_use",
    srcs = [
        "aou.trlc",
        "@score_baselibs//score/mw/log:docs/safety_analysis/aou_requirements.rst",
    ],
    package = "MwLogFrontendAoU",
    visibility = [
        "//score/mw/log/dependability:__subpackages__",
        "//score/mw/log/requirements:__subpackages__",
    ],
)
