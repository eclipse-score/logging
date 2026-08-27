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

load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

# ---------------------------------------------------------------------------
# mwfiltertest — used by mw_log verbose filter tests
# ---------------------------------------------------------------------------
cc_binary(
    name = "mwfiltertest",
    srcs = ["mwfiltertest.cpp"],
    deps = ["//score/mw/log"],
)

pkg_files(
    name = "mwfiltertest_bin_pkg",
    srcs = [":mwfiltertest"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "opt/test_apps/mwfiltertest/bin",
)

pkg_files(
    name = "mwfiltertest_config_pkg",
    srcs = ["etc/logging.json"],
    prefix = "opt/test_apps/mwfiltertest/etc",
)

pkg_filegroup(
    name = "mwfiltertest_filesystem_pkg",
    srcs = [
        ":mwfiltertest_bin_pkg",
        ":mwfiltertest_config_pkg",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_tar(
    name = "mwfiltertest_filesystem",
    srcs = [":mwfiltertest_filesystem_pkg"],
    visibility = ["//score/test/component:__subpackages__"],
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

pkg_files(
    name = "filtertest_bin_pkg",
    srcs = [":filtertest"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "opt/test_apps/filtertest/bin",
)

genrule(
    name = "filtertest_logging_json",
    srcs = ["etc/filtertest_logging.json"],
    outs = ["filtertest_etc/logging.json"],
    cmd = "cp $< $@",
)

pkg_files(
    name = "filtertest_config_pkg",
    srcs = [":filtertest_logging_json"],
    prefix = "opt/test_apps/filtertest/etc",
    strip_prefix = "filtertest_etc",
)

pkg_files(
    name = "filtertest_classid_pkg",
    srcs = ["etc/class-id.json"],
    prefix = "bmw/platform/opt/datarouter/etc",
    visibility = ["//score/test/component:__subpackages__"],
)

filegroup(
    name = "class_id_json",
    srcs = ["etc/class-id.json"],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_filegroup(
    name = "filtertest_filesystem_pkg",
    srcs = [
        ":filtertest_bin_pkg",
        ":filtertest_classid_pkg",
        ":filtertest_config_pkg",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_tar(
    name = "filtertest_filesystem",
    srcs = [":filtertest_filesystem_pkg"],
    visibility = ["//score/test/component:__subpackages__"],
)
