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
    name = "fg_persistent_logging_test_json",
    srcs = [
        "datarouter/persistent-logging.json",
        "datarouter/persistent-logging_test_1.json",
        "datarouter/persistent-logging_test_2.json",
        "datarouter/persistent-logging_test_3.json",
        "datarouter/persistent-logging_test_4.json",
        "datarouter/persistent-logging_test_5.json",
        "datarouter/persistent-logging_test_6.json",
    ],
    visibility = [
        "//platform/aas/pas/datarouterconf:__subpackages__",
        "//score/datarouter:__subpackages__",
    ],
)

filegroup(
    name = "fg_socketserver_config_test_json",
    srcs = [
        "datarouter/persistent-database-override_test.json",
    ],
    visibility = [
        "//platform/aas/mw:__pkg__",
        "//score/datarouter/test:__subpackages__",
    ],
)

filegroup(
    name = "fg_socket_server_log_channels_test_json",
    srcs = [
        "log-channels.json",
        "log-channels-default-threshold.json",
        "log-channels-default-thresold.json",
        "log-channels-empty.json",
        "log-channels-empty-channels.json",
        "log-channels-filtering-enabled.json",
        "log-channels-no-default-threshold.json",
        "log-channels-old-format.json",
        "log-channels-quotas.json",
        "log-channels-quotas-activated.json",
        "log-channels-quotas-deactivated.json",
        "log-channels-thresold-and-threshold.json",
        "log-channels-without-channels.json",
    ],
    visibility = ["//score/datarouter/test:__subpackages__"],
)

filegroup(
    name = "fg_dummy_file_transfer",
    srcs = ["dummy_file_transfer"],
    visibility = ["//score/datarouter/test:__subpackages__"],
)
