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
load("@rules_python//python:defs.bzl", "py_binary")
load("@score_docs_as_code//:docs.bzl", "docs")
load("@score_tooling//:defs.bzl", "copyright_checker", "setup_starpls")
load("@score_tooling//third_party/format:macros.bzl", "use_format_targets")

setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        ".bazelrc",
        ".github",
        "BUILD",
        "MODULE.bazel",
        "docs",
        "examples",
        "project_config.bzl",
        "score",
        "tests",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    exclusion = "//:.copyright_exclusions",
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

# Add target for formatting checks
use_format_targets()

exports_files([
    "MODULE.bazel",
    ".clang-tidy",
])

py_binary(
    name = "publish_dependable_element_reports",
    testonly = True,
    srcs = ["scripts/publish_dependable_element_reports.py"],
    data = ["//score/mw/log/dependability:dependable_element_mw_log_backend_doc"],
    main = "scripts/publish_dependable_element_reports.py",
)

# Creates all documentation targets:
# - `:docs` for building documentation at build-time
docs(
    data = [
        "@score_platform//:needs_json",
        "@score_process_description//:needs_json",
    ],
    source_dir = "docs",
)
