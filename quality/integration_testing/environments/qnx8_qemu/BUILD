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

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")

filegroup(
    name = "init_build",
    srcs = ["init.build"],
    visibility = ["//score/test/component:__subpackages__"],
)

filegroup(
    name = "system_build",
    srcs = ["system.build"],
    visibility = ["//score/test/component:__subpackages__"],
)

exports_files(
    [
        "dlt_config_qnx.json",
        "qemu_bridge_config.json",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_files(
    name = "etc_files_pkg",
    srcs = [
        "group",
        "hostname",
        "passwd",
        "profile",
    ],
    prefix = "etc",
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_files(
    name = "sshd_config_pkg",
    srcs = ["sshd_config"],
    attributes = pkg_attributes(mode = "0444"),
    prefix = "var/ssh",
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_files(
    name = "ssh_host_keys_pkg",
    srcs = [
        "ssh_host_rsa_key",
        "ssh_host_rsa_key.pub",
    ],
    attributes = pkg_attributes(
        gid = 0,
        mode = "0400",
        uid = 0,
    ),
    prefix = "var/ssh",
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_files(
    name = "scripts_pkg",
    srcs = [
        "network_setup.sh",
        "startup.sh",
    ],
    attributes = pkg_attributes(mode = "0700"),
    prefix = "etc",
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_files(
    name = "config_files_pkg",
    srcs = [
        "pci_hw.cfg",
        "pci_server.cfg",
        "qcrypto.conf",
    ],
    attributes = pkg_attributes(mode = "0444"),
    # pci-server/qcrypto look these up under /proc/boot (the IFS image root).
    prefix = "proc/boot",
    visibility = ["//score/test/component:__subpackages__"],
)

pkg_filegroup(
    name = "qnx8_qemu_env_pkg",
    srcs = [
        ":config_files_pkg",
        ":etc_files_pkg",
        ":scripts_pkg",
        ":ssh_host_keys_pkg",
        ":sshd_config_pkg",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)
