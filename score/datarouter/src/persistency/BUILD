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
    name = "interface",
    hdrs = ["i_persistent_dictionary.h"],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/datarouter:__subpackages__"],
)

cc_library(
    name = "persistency_factory",
    hdrs = [
        "persistent_dictionary_factory.hpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/datarouter:__subpackages__"],
    deps = [
        ":interface",
    ],
)

cc_library(
    name = "mock",
    testonly = True,
    hdrs = [
        "mock_persistent_dictionary.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/datarouter:__subpackages__"],
    deps = [
        ":interface",
        "@googletest//:gtest",
    ],
)
