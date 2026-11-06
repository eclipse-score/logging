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

load(
    "@score_tooling//bazel/rules/rules_score:rules_score.bzl",
    "dependability_analysis",
    "fault_trees",
    "safety_analysis",
)

# No fault tree diagrams yet; the rule requires the target.
fault_trees(
    name = "mw_log_backend_fault_trees",
)

safety_analysis(
    name = "mw_log_backend_safety_analysis",
    fault_trees = ":mw_log_backend_fault_trees",
    safety_measures = [
        "//score/mw/log/requirements/component_requirements",
        "//score/mw/log/requirements/safety_analysis:assumptions_of_use",
    ],
    visibility = ["//score/mw/log/dependability:__pkg__"],
)

dependability_analysis(
    name = "mw_log_backend_dependability_analysis",
    arch_design = "//score/mw/log/design/backend:mw_log_backend_architectural_design",
    safety_analysis = [":mw_log_backend_safety_analysis"],
    tags = ["manual"],
    visibility = ["//score/mw/log/dependability:__pkg__"],
)
