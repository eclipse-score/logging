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

# Top-level BUILD file for mw/log requirements.
# This aggregates all requirement sets (assumed_system, feature, component, safety_analysis).
#
# Scope decision (MVP, see trlc_migration_plan.md option (a)): TRLC is authored and
# type-checked here (`bazel test //score/mw/log/requirements:*`) as the traceability
# source for comp_req <-> feat_req <-> assumed_system_req and comp_req <-> test
# (via FullyVerifies/PartiallyVerifies in test code). Sphinx documentation continues
# to render from the existing hand-maintained
# docs/components/mw_log/requirements/requirements.rst; there is no
# //bazel/rules:sphinx_sources_bridge.bzl in this repo, and wiring TRLC->RST
# auto-generation into the flat docs() target is deferred (would require adopting
# component()/dependable_element(), a much larger scope -- see the plan's "Open
# decision"). Keep the two files in sync by hand until that decision is made.

# Top-level BUILD file for mw/log requirements.
# This aggregates all requirement sets (assumed_system, feature, component, safety_analysis).
#
# Scope decision (MVP, see trlc_migration_plan.md option (a)): TRLC is authored and
# type-checked here (`bazel test //score/mw/log/requirements:*`) as the traceability
# source for comp_req <-> feat_req <-> assumed_system_req and comp_req <-> test
# (via FullyVerifies/PartiallyVerifies in test code). Sphinx documentation continues
# to render from the existing hand-maintained
# docs/components/mw_log/requirements/requirements.rst; there is no
# //bazel/rules:sphinx_sources_bridge.bzl in this repo, and wiring TRLC->RST
# auto-generation into the flat docs() target is deferred (would require adopting
# component()/dependable_element(), a much larger scope -- see the plan's "Open
# decision"). Keep the two files in sync by hand until that decision is made.
#
# POC (local_path_override to ../tooling in //MODULE.bazel): the resolved score_tooling
# now supports deps=/ref_package=/package= and emits TrlcProviderInfo + its own
# <name>_test per requirement target (see @score_tooling//bazel/rules/rules_score/private/
# requirements.bzl), so a manual trlc_requirements/trlc_requirements_test aggregator is no
# longer needed -- component_requirements's own generated test already validates the full
# chain transitively via its deps on feature_requirements, which in turn deps on
# assumed_system_requirements. Kept as an alias so `bazel test //score/mw/log/requirements:*`
# still runs it under the same name as before.
alias(
    name = "mw_log_requirements_test",
    actual = "//score/mw/log/requirements/component_requirements:component_requirements_test",
    visibility = ["//visibility:public"],
)


# score/mw/log/requirements/safety_analysis/aou.trlc (ScoreReq.AoU) is intentionally NOT
# included above: the vendored //third_party/score_requirement_model schema (mirroring the
# pinned score_tooling 1.1.2's bundled one) does not define an `AoU` type at all -- this is a
# version-specific gap (score_tooling 2.0.2, used e.g. by score_communication, does define it),
# pre-existing and unrelated to this MVP's comp_req/feat_req/assumed_system_req scope. Fixing
# it is safety-analysis follow-up work, not part of component/test traceability.
