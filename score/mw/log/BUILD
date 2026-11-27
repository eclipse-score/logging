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

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@score_docs_as_code//:docs.bzl", "docs_bundle")
load("@score_tooling//bazel/rules/rules_score:rules_score.bzl", "architectural_design")

COMPILER_WARNING_FEATURES = [
    "treat_warnings_as_errors",
    "additional_warnings",
    "strict_warnings",
]

cc_library(
    name = "log",
    deprecation = "Usage of target '@score_logging//score/mw/log' is deprecated. Please use '@score_baselibs//score/mw/log' instead.",
    features = COMPILER_WARNING_FEATURES,
    tags = ["FFI"],
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/log/backend:file",
        "//score/mw/log/backend:remote",
        "@score_baselibs//score/mw/log:console",
        "@score_baselibs//score/mw/log:frontend",
    ] + select({
        "@platforms//os:qnx": ["//score/mw/log/backend:slog"],
        "//conditions:default": [],
    }),
)

# Component documentation, mounted by the root docs(). The bundle sits at the
# component package boundary, so its code (and coverage) contains the mw::log
# Backend implementation only, including the Rust bridge. The slog backend is
# QNX only and cannot be built on the host, so it is not listed.
docs_bundle(
    name = "docs_bundle",
    code_targets = [
        "//score/mw/log/backend:custom",
        "//score/mw/log/backend:file",
        "//score/mw/log/backend:remote",
        "//score/mw/log/backend:syslog",
        "//score/mw/log/legacy_non_verbose_api",
        "//score/mw/log/rust/score_log_bridge",
        "//score/mw/log/rust/score_log_bridge_cpp_init",
    ],
    primary_need_id = "comp__mw_log_backend",
    source_dir = "docs",
    visibility = ["//visibility:public"],
)

architectural_design(
    name = "mw_log_backend_architectural_design",
    dynamic = [
        "docs/detailed_design/_assets/datarouter_backend_datarouterbackend.puml",
        "docs/detailed_design/_assets/datarouter_message_client_impl_connecttodatarouter.puml",
        "docs/detailed_design/_assets/shared_memory_reader_read.puml",
        "docs/detailed_design/_assets/shared_memory_writer_allocandwrite.puml",
        "docs/detailed_design/_assets/verbose_logging_sequence.puml",
        "docs/detailed_design/_assets/syslog_backend_sequence.puml",
    ],
    internal_api = [
        "docs/detailed_design/_assets/datarouter_class_diagram.puml",
        "docs/detailed_design/_assets/inter_process_communication.puml",
        "docs/detailed_design/_assets/mw_log_datarouter_recorder.puml",
        "docs/detailed_design/_assets/mw_log_shared_memory_reader.puml",
        "docs/detailed_design/_assets/write_factory_design.puml",
        "docs/detailed_design/_assets/mw_log_file_backend.puml",
        "docs/detailed_design/_assets/mw_log_recorders.puml",
    ],
    maturity = "development",
    static = [
        "docs/detailed_design/datarouter_backend/index.md",
        "docs/detailed_design/_assets/verbose_logging_static.puml",
        "docs/detailed_design/file_output_backend.md",
    ],
    visibility = ["//visibility:public"],
)
