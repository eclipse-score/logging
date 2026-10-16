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
load("@score_tooling//:defs.bzl", "copyright_checker", "setup_starpls")
load("@score_tooling//bazel/rules/rules_score:rules_score.bzl", "architectural_design")
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

# Creates all documentation targets:
# - `:docs` for building documentation at build-time
docs(
    data = [
        "@score_platform//:needs_json",
        "@score_process_description//:needs_json",
    ],
    source_dir = "docs",
)

# Detailed design of the mw::log backend. The sources are part of the docs tree.
architectural_design(
    name = "mw_log_backend_architectural_design",
    dynamic = [
        "docs/components/mw_log/detailed_design/_assets/datarouter_backend_datarouterbackend.puml",
        "docs/components/mw_log/detailed_design/_assets/datarouter_message_client_impl_connecttodatarouter.puml",
        "docs/components/mw_log/detailed_design/_assets/shared_memory_reader_read.puml",
        "docs/components/mw_log/detailed_design/_assets/shared_memory_writer_allocandwrite.puml",
        "docs/components/mw_log/detailed_design/_assets/slot_drainer_sequence_design.puml",
        "docs/components/mw_log/detailed_design/_assets/syslog_backend_sequence.puml",
        "docs/components/mw_log/detailed_design/_assets/verbose_logging_sequence.puml",
    ],
    static = [
        "docs/components/mw_log/detailed_design/_assets/datarouter_class_diagram.puml",
        "docs/components/mw_log/detailed_design/_assets/backend_registration.puml",
        "docs/components/mw_log/detailed_design/_assets/inter_process_communication.puml",
        "docs/components/mw_log/detailed_design/_assets/mw_log_datarouter_recorder.puml",
        "docs/components/mw_log/detailed_design/_assets/mw_log_file_backend.puml",
        "docs/components/mw_log/detailed_design/_assets/mw_log_recorders.puml",
        "docs/components/mw_log/detailed_design/_assets/mw_log_shared_memory_reader.puml",
        "docs/components/mw_log/detailed_design/_assets/verbose_logging_static.puml",
        "docs/components/mw_log/detailed_design/_assets/write_factory_design.puml",
        "docs/components/mw_log/detailed_design/datarouter_backend/index.md",
        "docs/components/mw_log/detailed_design/file_output_backend.md",
        "docs/components/mw_log/detailed_design/slog_backend.md",
        "docs/components/mw_log/detailed_design/backend_registration.md",
        "docs/components/mw_log/detailed_design/syslog_backend.md",
    ],
    # score_tooling 2.x parses every .puml included by this target.
    visibility = ["//visibility:public"],
)
