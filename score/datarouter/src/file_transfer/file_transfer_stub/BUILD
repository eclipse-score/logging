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

load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

cc_library(
    name = "file_transfer_handler_factory_stub",
    hdrs = [
        "file_transfer_handler_factory_stub.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter:__pkg__",
        "//score/datarouter/test:__subpackages__",
    ],
    deps = [
        ":file_transfer_stream_handler_stub",
        "//score/datarouter:logparser",
        "//score/datarouter/src/file_transfer:file_transfer_handler_factory",
    ],
)

cc_library(
    name = "file_transfer_stream_handler_stub",
    hdrs = [
        "file_transfer_stream_handler_stub.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter:__pkg__",
    ],
    deps = [
        "//score/datarouter:dltprotocol",
        "//score/datarouter:dltserver_common",
        "//score/datarouter:log",
        "//score/datarouter:logparser",
        "@score_baselibs//score/language/futurecpp",
    ],
)
