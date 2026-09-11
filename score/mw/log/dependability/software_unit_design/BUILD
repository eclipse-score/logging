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

# Minimal unit_design() stubs: the static/dynamic diagrams already exist at
# //score/mw/log/design/backend and are referenced at the dependable_element's
# architectural_design level; these units don't (yet) have unit-specific
# design docs beyond that.
load("@score_tooling//bazel/rules/rules_score:rules_score.bzl", "unit_design")

unit_design(
    name = "file_recorder_unit_design",
    visibility = ["//score/mw/log/detail/file_recorder:__pkg__"],
)

unit_design(
    name = "data_router_backend_unit_design",
    visibility = ["//score/mw/log/detail/data_router:__pkg__"],
)

unit_design(
    name = "wait_free_producer_queue_unit_design",
    visibility = ["//score/mw/log/detail/wait_free_producer_queue:__pkg__"],
)

unit_design(
    name = "shared_memory_unit_design",
    visibility = ["//score/mw/log/detail/data_router/shared_memory:__pkg__"],
)
