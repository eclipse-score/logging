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
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "file_transfer_stream_handler_factory",
    hdrs = [
        "file_transfer_stream_handler_factory.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test:__subpackages__",
    ],
    deps = [
        ":file_transfer_stream_handler",
        "//score/datarouter/src/file_transfer:file_transfer_handler_factory",
        "@score_logging//score/datarouter:logparser",
    ],
)

[
    cc_library(
        name = "file_transfer_stream_handler" + name,
        testonly = test_only,
        srcs = [
            "filetransfer_stream.cpp",
        ],
        hdrs = [
            "filetransfer_stream.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        visibility = [
            "@score_logging//score/datarouter:__pkg__",
        ],
        deps = [
            "@score_baselibs//score/mw/log",
            "@score_baselibs//score/os:stat",
            "@score_baselibs//score/os:stdio",
            "@score_baselibs//score/os/utils:thread",
            "@score_logging//score/datarouter:dltprotocol",
            "@score_logging//score/datarouter:dltserver_common",
            "@score_logging//score/datarouter:logparser",
            udp_dep,
            "//score/datarouter/dlt_filetransfer_trigger_lib:filetransfer_message_types",
            "@score_baselibs//score/language/futurecpp",
        ],
    )
    for name, udp_dep, test_only in [
        ("", "@score_logging//score/datarouter:udp_stream_output", False),
        ("_testing", "@score_logging//score/datarouter:udpoutput_mock", True),
    ]
]
