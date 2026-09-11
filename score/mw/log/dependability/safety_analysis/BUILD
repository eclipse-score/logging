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
    "fmea",
)

# PLACEHOLDER (POC only) -- see failure_modes.trlc/control_measures.trlc notes.
fmea(
    name = "mw_log_backend_fmea",
    arch_design = "//score/mw/log/design/backend:mw_log_backend_architectural_design",
    controlmeasures = ["control_measures.trlc"],
    failuremodes = ["failure_modes.trlc"],
    visibility = ["//score/mw/log/dependability:__pkg__"],
)

dependability_analysis(
    name = "mw_log_backend_dependability_analysis",
    arch_design = "//score/mw/log/design/backend:mw_log_backend_architectural_design",
    fmea = [":mw_log_backend_fmea"],
    tags = ["manual"],
    visibility = ["//score/mw/log/dependability:__pkg__"],
)
