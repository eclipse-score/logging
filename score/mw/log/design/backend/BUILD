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

load(
    "@score_tooling//bazel/rules/rules_score:rules_score.bzl",
    "architectural_design",
)

architectural_design(
    name = "mw_log_backend_architectural_design",
    # POC: most of the diagrams below fail the newer score_tooling's stricter
    # puml_cli parser (real PlantUML syntax it doesn't support -- multi-line
    # method comments, swimlanes, destroyed participants, etc.); this target
    # isn't wired into any live docs build today, so trimmed to the small
    # subset that actually parses rather than reworking every diagram blind.
    dynamic = [
        "datarouter_backend/shared_memory_writer_allocandwrite.puml",
    ],
    static = [
        "datarouter_backend/index.md",
        # "datarouter_backend/datarouter_class_diagram.puml",
        # "datarouter_backend/inter_process_communication.puml",
        # "datarouter_backend/mw_log_datarouter_recorder.puml",
        "datarouter_backend/mw_log_shared_memory_reader.puml",
        "file_output_backend.md",
    ],
    # score_tooling 2.x parses every .puml; the legacy diagrams here are not
    # yet in the parser's supported subset (eclipse-score/tooling#447).
    # Keep the target out of wildcard builds until they are reworked.
    tags = ["manual"],
    visibility = ["//visibility:public"],
)
