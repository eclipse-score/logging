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
load("@rules_python//python:defs.bzl", "py_binary", "py_test")
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

_DEPENDABLE_ELEMENT = "dependable_element_mw_log_backend_doc"

# Public URL folder; docs/dependable_elements/index.rst links to it.
_PUBLISH_NAME = "mw_log_backend"

_DEPENDABLE_ELEMENT_DOC = "//score/mw/log/dependability:" + _DEPENDABLE_ELEMENT

# Must run after `:docs`, which owns and may wipe `_build`.
py_binary(
    name = "publish_dependable_element_reports",
    testonly = True,
    srcs = ["scripts/publish_dependable_element_reports.py"],
    data = [_DEPENDABLE_ELEMENT_DOC],
    env = {
        "DEPENDABLE_ELEMENT_HTML": "$(rlocationpath " + _DEPENDABLE_ELEMENT_DOC + ")",
        "DEPENDABLE_ELEMENT_NAME": _PUBLISH_NAME,
    },
    main = "scripts/publish_dependable_element_reports.py",
    deps = ["@rules_python//python/runfiles"],
)

py_test(
    name = "publish_dependable_element_reports_test",
    srcs = [
        "scripts/publish_dependable_element_reports.py",
        "scripts/publish_dependable_element_reports_test.py",
    ],
    main = "scripts/publish_dependable_element_reports_test.py",
    deps = ["@rules_python//python/runfiles"],
)

py_test(
    name = "generate_dependency_report_test",
    srcs = [
        "scripts/generate_dependency_report.py",
        "scripts/generate_dependency_report_test.py",
    ],
    main = "scripts/generate_dependency_report_test.py",
)

# Creates all documentation targets:
# - `:docs` for building documentation at build-time
docs(
    data = glob(["scripts/dependency_reports/dependency_report_*.json"]) + [
        "scripts/generate_dependency_report.py",
        "@score_platform//:needs_json",
        "@score_process_description//:needs_json",
    ],
    source_dir = "docs",
)
