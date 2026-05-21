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

load("//score/test/component:defs.bzl", "py_logging_itf_test")

py_logging_itf_test(
    name = "test_datarouter_filters",
    srcs = ["test_datarouter_filters.py"],
    filesystem = "//score/test/component/filters_app:filtertest_filesystem",
    target_compatible_with = select({
        "//score/datarouter/build_configuration_flags:config_datarouter_nonverbose_dlt": [],
        "//conditions:default": ["@platforms//:incompatible"],
    }),
    deps = ["@score_itf//score/itf/plugins/dlt"],
)
