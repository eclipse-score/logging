# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
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

load("@bazel_skylib//rules:copy_file.bzl", "copy_file")
load(
    "@score_tooling//bazel/rules/rules_score:rules_score.bzl",
    "assumptions_of_use",
)

copy_file(
    name = "aou_requirements_rst",
    src = "@score_baselibs//score/mw/log:docs/safety_analysis/aou_requirements.rst",
    out = "aou_requirements.rst",
)

assumptions_of_use(
    name = "assumptions_of_use",
    srcs = ["aou_requirements.rst"],
    visibility = [
        "//score/mw/log/dependability:__subpackages__",
        "//score/mw/log/requirements:__subpackages__",
    ],
)
