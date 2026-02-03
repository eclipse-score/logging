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

## ===========================================================================
##  File transfer
## ---------------------------------------------------------------------------

cc_library(
    name = "dlt_filetransfer_trigger_lib",
    srcs = ["src/file_transfer.cpp"],
    hdrs = [
        "include/file_transfer.h",
        "include/ifile_transfer.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/pas/crash_handler:__pkg__",
        "//platform/aas/pas/datarouterconf/src/services:__pkg__",
        "//platform/aas/test/pas:__subpackages__",
        "//score/datarouter/test:__subpackages__",
        "@score_baselibs//score/logging:__pkg__",
    ],
    deps = [
        "filetransfer_message_types",
        "//score/datarouter:log",
    ],
)

cc_library(
    name = "filetransfer_message_types",
    hdrs = [
        "filetransfer_message.h",
        "filetransfer_message_trace.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//platform/aas/pas/crash_handler:__subpackages__",
        "//platform/aas/pas/datarouterconf/src/filetransfer:__subpackages__",
        "//score/datarouter:__subpackages__",
        "//score/mw/log/legacy_non_verbose_api:__pkg__",
    ],
    deps = [
        "//score/datarouter:dltprotocol",
    ],
)

cc_library(
    name = "filetransfer_mock",
    testonly = 1,
    hdrs = [
        "mock/filetransfer_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        ":dlt_filetransfer_trigger_lib",
        "@googletest//:gtest",
    ],
)
