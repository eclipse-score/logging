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
    name = "datarouter_architectural_design",
    dynamic = [
        "uml/client_session_interaction_sequence.puml",
        "uml/sequence_early_disconnect_race.puml",
        "uml/seq_kvs_threads_sequence_diagram.puml",
    ],
    static = [
        "uml/context-sw-units.puml",
        "uml/datarouter_units.puml",
        "uml/detail-ipc.puml",
        "uml/file_transfer_feature_class_diagram.puml",
        "uml/package-datarouter.puml",
    ],
    visibility = ["//visibility:public"],
)
