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

# POC (option 2, see trlc_migration_plan.md): mw::log backend as a full
# rules_score SEooC. maturity="development" and the placeholder FMEA mean this
# is not yet certification evidence -- see safety_analysis/*.trlc notes.
load(
    "@score_tooling//bazel/rules/rules_score:rules_score.bzl",
    "component",
    "dependable_element",
)

component(
    name = "component_mw_log_backend",
    components = [
        "//score/mw/log/detail/data_router:data_router_backend_unit",
        "//score/mw/log/detail/data_router/shared_memory:shared_memory_unit",
        "//score/mw/log/detail/file_recorder:file_recorder_unit",
        "//score/mw/log/detail/wait_free_producer_queue:wait_free_producer_queue_unit",
    ],
    requirements = [
        "//score/mw/log/requirements/component_requirements",
        "//score/mw/log/requirements/feature_requirements",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    test_case_coverage_lock = "test_case_coverage.lock.yaml",
    visibility = ["//visibility:public"],
)

dependable_element(
    name = "dependable_element_mw_log_backend",
    architectural_design = ["//score/mw/log/design/backend:mw_log_backend_architectural_design"],
    assumptions_of_use = ["//score/mw/log/requirements/safety_analysis:assumptions_of_use"],
    components = [":component_mw_log_backend"],
    dependability_analysis = ["//score/mw/log/dependability/safety_analysis:mw_log_backend_dependability_analysis"],
    integrity_level = "B",
    maturity = "development",
    requirements = [
        "//score/mw/log/requirements/feature_requirements",
        "//score/mw/log/requirements/assumed_system_requirements",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    tests = [
        "//score/test/component/mw_log:test_mw_log",
    ],
    visibility = ["//visibility:public"],
)
