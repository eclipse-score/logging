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
    name = "file_transfer_handler_factory_stub",
    hdrs = [
        "file_transfer_handler_factory_stub.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test:__subpackages__",
        "@score_logging//score/datarouter:__pkg__",
    ],
    deps = [
        ":file_transfer_stream_handler_stub",
        "//score/datarouter/src/file_transfer:file_transfer_handler_factory",
        "@score_logging//score/datarouter:logparser",
    ],
)

cc_library(
    name = "file_transfer_stream_handler_stub",
    hdrs = [
        "file_transfer_stream_handler_stub.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "@score_logging//score/datarouter:__pkg__",
    ],
    deps = [
        "@score_baselibs//score/language/futurecpp",
        "@score_logging//score/datarouter:dltprotocol",
        "@score_logging//score/datarouter:dltserver_common",
        "@score_logging//score/datarouter:log",
        "@score_logging//score/datarouter:logparser",
    ],
)
