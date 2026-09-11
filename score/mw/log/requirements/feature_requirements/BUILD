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
    "feature_requirements",
)

# Sourced live from score's public docs (POC: score_platform local_path_override
# in //MODULE.bazel) instead of a hand-authored .trlc, via rules_score's
# rst_to_trlc bridge -- avoids duplicating the same feature requirements in
# two places. ref_package/package name the ASR tier and this file's own tier
# respectively (see requirements.bzl's score_requirements_rule doc).
feature_requirements(
    name = "feature_requirements",
    srcs = ["@score_platform//:docs/features/log_and_trace/logging/requirements/index.rst"],
    deps = ["//score/mw/log/requirements/assumed_system_requirements"],
    package = "MwLogFeature",
    ref_package = "MwLogAssumedSystemRequirements",
    visibility = [
        "//score/mw/log/dependability:__subpackages__",
        "//score/mw/log/requirements:__subpackages__",
    ],
)

