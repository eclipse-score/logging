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
    name = "fake_recorder",
    testonly = True,
    srcs = ["fake_recorder.cpp"],
    hdrs = ["fake_recorder.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        "//score/datarouter/lib/synchronized:synchronized_utility",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log:recorder",
    ],
)
