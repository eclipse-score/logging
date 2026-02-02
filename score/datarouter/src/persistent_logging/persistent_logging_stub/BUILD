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

cc_library(
    name = "sysedr_stub",
    srcs = [
        "stub_sysedr_factory.cpp",
        "stub_sysedr_handler.cpp",
    ],
    hdrs = [
        "stub_sysedr_factory.h",
        "stub_sysedr_handler.h",
    ],
    visibility = ["//score/datarouter:__subpackages__"],
    deps = [
        "//score/datarouter:log",
        "//score/datarouter/src/persistent_logging:sysedr_factory",
        "//score/datarouter/src/persistent_logging:sysedr_handler_interface",
    ],
)
