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

filegroup(
    name = "init_build",
    srcs = ["init.build"],
    visibility = ["//score/test/component:__subpackages__"],
)

filegroup(
    name = "qnx8_qemu_env",
    srcs = glob([
        "*.sh",
        "*.cfg",
        "*.conf",
        "ssh_host_rsa_key*",
    ]) + [
        "group",
        "hostname",
        "passwd",
        "profile",
        "sshd_config",
        "system.build",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)

exports_files(
    [
        "dlt_config_qnx.json",
        "qemu_bridge_config.json",
    ],
    visibility = ["//score/test/component:__subpackages__"],
)
