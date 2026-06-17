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
# ********************************************************************************
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
# ********************************************************************************

load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

# ---------------------------------------------------------------------------
# mwfiltertest — used by mw_log verbose filter tests
# ---------------------------------------------------------------------------
cc_binary(
    name = "mwfiltertest",
    srcs = ["mwfiltertest.cpp"],
    deps = ["//score/mw/log"],
)

pkg_tar(
    name = "mwfiltertest_bin_pkg",
    srcs = [":mwfiltertest"],
    package_dir = "opt/test_apps/mwfiltertest/bin",
)

pkg_tar(
    name = "mwfiltertest_config_pkg",
    srcs = ["etc/logging.json"],
    package_dir = "opt/test_apps/mwfiltertest/etc",
)

pkg_tar(
    name = "mwfiltertest_filesystem",
    visibility = ["//score/test/component:__subpackages__"],
    deps = [
        ":mwfiltertest_bin_pkg",
        ":mwfiltertest_config_pkg",
    ],
)

# ---------------------------------------------------------------------------
# filtertest — used by datarouter non-verbose filter tests
# ---------------------------------------------------------------------------
cc_binary(
    name = "filtertest",
    srcs = [
        "filtertest.cpp",
        "filtertest.h",
    ],
    deps = [
        "//score/mw/log",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/static_reflection_with_serialization/visitor",
    ],
)

pkg_tar(
    name = "filtertest_bin_pkg",
    srcs = [":filtertest"],
    package_dir = "opt/test_apps/filtertest/bin",
)

genrule(
    name = "filtertest_logging_json",
    srcs = ["etc/filtertest_logging.json"],
    outs = ["filtertest_etc/logging.json"],
    cmd = "cp $< $@",
)

pkg_tar(
    name = "filtertest_config_pkg",
    srcs = [":filtertest_logging_json"],
    package_dir = "opt/test_apps/filtertest/etc",
    strip_prefix = "filtertest_etc",
)

pkg_tar(
    name = "filtertest_classid_pkg",
    srcs = ["etc/class-id.json"],
    package_dir = "bmw/platform/opt/datarouter/etc",
    visibility = ["//score/test/component:__subpackages__"],
)

filegroup(
    name = "class_id_json",
    srcs = ["etc/class-id.json"],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_tar(
    name = "filtertest_filesystem",
    visibility = ["//score/test/component:__subpackages__"],
    deps = [
        ":filtertest_bin_pkg",
        ":filtertest_classid_pkg",
        ":filtertest_config_pkg",
    ],
)
