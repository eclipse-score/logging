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

cc_library(
    name = "vlan",
    hdrs = [
        "vlan.h",
    ],
    defines = select({
        "//score/datarouter/build_configuration_flags:enable_local_vlan": ["USE_LOCAL_VLAN"],
        "//conditions:default": [],
    }),
    features = [
        "treat_warnings_as_errors",
        "additional_warnings",
        "strict_warnings",
    ],
    visibility = ["//score/datarouter:__pkg__"],
    deps = select({
        "//score/datarouter/build_configuration_flags:enable_local_vlan": [":vlan_local"],
        "//conditions:default": [":vlan_local"],
    }),
)

cc_library(
    name = "vlan_local",
    srcs = ["vlan_local.cpp"],
    hdrs = ["vlan_local.h"],
    features = [
        "treat_warnings_as_errors",
        "additional_warnings",
        "strict_warnings",
    ],
    deps = [
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/os:socket",
    ],
)
