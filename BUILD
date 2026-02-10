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
load("@score_docs_as_code//:docs.bzl", "docs")
load("@score_tooling//:defs.bzl", "copyright_checker", "setup_starpls", "use_format_targets")

setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        ".github",
        "docs",
        "examples",
        # "score", # Disabled until antonkri is back from vacations as we cannot get approval on some repo parts
        "tests",
        "//:.bazelrc",
        "//:BUILD",
        "//:MODULE.bazel",
        "//:project_config.bzl",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

# Add target for formatting checks
use_format_targets()

exports_files([
    "MODULE.bazel",
])

# Creates all documentation targets:
# - `:docs` for building documentation at build-time
docs(
    data = [
        # "@score_platform//:needs_json",
        # "@score_process//:needs_json",
    ],
    source_dir = "docs",
)
