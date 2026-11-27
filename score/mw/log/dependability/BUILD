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
# This development model includes frontend and backend components. Its backend
# scope still includes file and syslog units and is restricted to Linux.
# The development maturity and placeholder FMEA are not certification evidence.
# The current baselibs release needs a local override until the frontend component is released.
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
        "//score/mw/log/detail/syslog:syslog_backend_unit",
        "//score/mw/log/detail/utils/signal_handling:signal_handling_unit",
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
    architectural_design = [
        "//score/mw/log/design/backend:mw_log_backend_architectural_design",
        "@score_baselibs//score/mw/log:mw_log_architectural_design",
    ],
    components = [
        ":component_mw_log_backend",
        "@score_baselibs//score/mw/log:component_mw_log_frontend",
    ],
    dependability_analysis = ["//score/mw/log/dependability/safety_analysis:mw_log_backend_dependability_analysis"],
    integrity_level = "B",
    maturity = "development",
    requirements = [
        "//score/mw/log/requirements/feature_requirements",
        "//score/mw/log/requirements/assumed_system_requirements",
        "@score_baselibs//score/mw/log:mw_log_feature_requirements",
        "@score_baselibs//score/mw/log:mw_log_assumed_system_requirements",
    ],
    target_compatible_with = ["@platforms//os:linux"],
    tests = [
        "//score/test/component/mw_log:test_mw_log",
    ],
    visibility = ["//visibility:public"],
)
