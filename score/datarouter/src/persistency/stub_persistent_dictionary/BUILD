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
    name = "stub",
    srcs = [
        "stub_persistent_dictionary.cpp",
    ],
    hdrs = [
        "stub_persistent_dictionary.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/datarouter:__subpackages__"],
    deps = [
        "//score/datarouter/src/persistency:interface",
    ],
)

cc_library(
    name = "stub_persistent_dictionary_factory",
    srcs = [
        "stub_persistent_dictionary_factory.cpp",
    ],
    hdrs = [
        "stub_persistent_dictionary_factory.h",
    ],
    visibility = ["//score/datarouter:__subpackages__"],
    deps = [
        ":stub",
        "//score/datarouter/src/persistency:persistency_factory",
    ],
)
