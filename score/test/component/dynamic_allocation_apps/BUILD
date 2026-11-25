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

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

cc_binary(
    name = "mw_log_golden_app",
    srcs = ["src/mw_log_golden_app.cpp"],
    deps = ["//score/mw/log"],
)

cc_binary(
    name = "mw_log_allocations",
    srcs = ["src/mw_log_allocations.cpp"],
    deps = ["//score/mw/log"],
)

cc_binary(
    name = "non_verbose_golden_app",
    srcs = ["src/non_verbose_golden_app.cpp"],
    deps = [
        "//score/mw/log",
        "//score/mw/log/legacy_non_verbose_api",
        "@score_baselibs//score/static_reflection_with_serialization/visitor",
    ],
)

cc_binary(
    name = "non_verbose_allocations",
    srcs = ["src/non_verbose_allocations.cpp"],
    deps = [
        "//score/mw/log",
        "//score/mw/log/legacy_non_verbose_api",
        "@score_baselibs//score/static_reflection_with_serialization/visitor",
    ],
)

pkg_files(
    name = "dynamic_allocation_apps_bin_pkg",
    srcs = [
        ":mw_log_allocations",
        ":mw_log_golden_app",
        ":non_verbose_allocations",
        ":non_verbose_golden_app",
    ],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "opt/test_apps/dynamic_allocation_apps/bin",
)

pkg_files(
    name = "dynamic_allocation_apps_lib_pkg",
    srcs = ["//score/test/component/malloc_profiler:malloc-profiler"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "opt/test_apps/dynamic_allocation_apps/lib",
    renames = {"//score/test/component/malloc_profiler:malloc-profiler": "libmalloc-profiler.so"},
)

pkg_files(
    name = "dynamic_allocation_apps_config_pkg",
    srcs = ["etc/logging.json"],
    prefix = "opt/test_apps/dynamic_allocation_apps/etc",
)

pkg_filegroup(
    name = "dynamic_allocation_apps_filesystem_pkg",
    srcs = [
        ":dynamic_allocation_apps_bin_pkg",
        ":dynamic_allocation_apps_config_pkg",
        ":dynamic_allocation_apps_lib_pkg",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_tar(
    name = "dynamic_allocation_apps_filesystem",
    srcs = [":dynamic_allocation_apps_filesystem_pkg"],
    visibility = ["//score/test/component:__subpackages__"],
)
