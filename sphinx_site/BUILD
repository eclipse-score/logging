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
load("@score_tooling//bazel/rules/rules_score:rules_score.bzl", "sphinx_module")

# Renders docs/ plus the generated mw::log dependable element documentation
# (requirements, architecture, dependability analysis, LOBSTER traceability).
sphinx_module(
    name = "sphinx_doc",
    testonly = True,
    srcs = ["index.rst"],
    docs_library_deps = [
        "//:logging_docs",
        "//score/mw/log/dependability:dependable_element_mw_log_backend_rst",
    ],
    exec_compatible_with = ["@platforms//os:linux"],
    index = "index.rst",
    target_compatible_with = ["@platforms//os:linux"],
)
