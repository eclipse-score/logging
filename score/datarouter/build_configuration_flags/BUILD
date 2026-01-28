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

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

bool_flag(
    name = "persistent_logging",
    build_setting_default = False,
)

config_setting(
    name = "config_persistent_logging",
    flag_values = {
        ":persistent_logging": "True",
    },
    visibility = [
        "//platform/aas/test/pas/datarouterconf:__subpackages__",
        "@score_logging//score/datarouter:__subpackages__",
    ],
)

bool_flag(
    name = "persistent_config_feature_enabled",
    build_setting_default = True,
)

config_setting(
    name = "config_persistent_configuration",
    flag_values = {":persistent_config_feature_enabled": "True"},
    visibility = ["@score_logging//score/datarouter:__subpackages__"],
)

bool_flag(
    name = "enable_nonverbose_dlt",
    build_setting_default = True,
)

config_setting(
    name = "config_datarouter_nonverbose_dlt",
    flag_values = {
        ":enable_nonverbose_dlt": "True",
    },
    visibility = [
        "@score_logging//score/datarouter:__subpackages__",
    ],
)

bool_flag(
    name = "enable_dynamic_configuration",
    build_setting_default = True,
)

config_setting(
    name = "config_datarouter_dynamic_configuration",
    flag_values = {
        ":enable_dynamic_configuration": "True",
    },
    visibility = [
        "@score_logging//score/datarouter:__subpackages__",
    ],
)

bool_flag(
    name = "file_transfer",
    build_setting_default = True,
)

config_setting(
    name = "config_file_transfer",
    flag_values = {
        ":file_transfer": "True",
    },
    visibility = [
        "@score_logging//score/datarouter:__subpackages__",
    ],
)

bool_flag(
    name = "enable_thin_network_sender",
    build_setting_default = False,
)

config_setting(
    name = "config_thin_network_sender",
    flag_values = {
        ":enable_thin_network_sender": "False",
    },
    visibility = [
        "@score_logging//score/datarouter:__subpackages__",
    ],
)

bool_flag(
    name = "use_local_vlan",
    build_setting_default = False,
)

config_setting(
    name = "enable_local_vlan",
    flag_values = {":use_local_vlan": "True"},
)
