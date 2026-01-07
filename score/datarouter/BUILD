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

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

## ===========================================================================
## dltprotocol
## ---------------------------------------------------------------------------

cc_library(
    name = "dltprotocol",
    srcs = [
        "src/dlt/dltid_converter.cpp",
    ],
    hdrs = [
        "include/dlt/dlt_common.h",
        "include/dlt/dlt_headers.h",
        "include/dlt/dlt_protocol.h",
        "include/dlt/dlt_types.h",
        "include/dlt/dltid.h",
        "include/dlt/dltid_converter.h",
        "include/dlt/logentry_trace.h",
        "include/dlt/plogfilterdesc.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//score/datarouter/dlt_filetransfer_trigger_lib:__pkg__",
        "//score/datarouter/file_transfer:__subpackages__",
        "//score/datarouter/persistent_logging/persistent_logging:__pkg__",
        "//score/datarouter/test:__subpackages__",
    ],
    deps = [
        "//score/mw/log/detail/common:log_entry_deserialize",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/os/utils:path",
    ],
)

## ===========================================================================
## Unix domain socket
## ---------------------------------------------------------------------------

cc_library(
    name = "unixdomain_common",
    srcs = [
        "src/unix_domain/unix_domain_common.cpp",
    ],
    hdrs = [
        "include/unix_domain/unix_domain_common.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//platform/aas/pas/datarouterconf:__subpackages__",
    ],
    deps = [
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/os:socket",
        "@score_baselibs//score/os/utils:signal",
    ],
)

cc_library(
    name = "unixdomain_client",
    srcs = [
        "src/unix_domain/unix_domain_client.cpp",
    ],
    hdrs = [
        "include/unix_domain/unix_domain_client.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//visibility:private"],
    deps = [
        ":unixdomain_common",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/os:pthread",
    ],
)

cc_library(
    name = "unixdomain_server",
    srcs = [
        "src/unix_domain/unix_domain_server.cpp",
    ],
    hdrs = [
        "include/unix_domain/unix_domain_server.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//score/datarouter/dynamic_configuration:__subpackages__",
        "//score/datarouter/test:__subpackages__",
    ],
    deps = [
        ":unixdomain_common",
        "//score/datarouter/dynamic_configuration/i_session",
        "@score_baselibs//score/os:pthread",
        "@score_baselibs//score/os:sys_poll",
    ],
)

cc_library(
    name = "unixdomain_testing",
    srcs = [
        "src/unix_domain/unix_domain_server.cpp",
    ],
    hdrs = [
        "include/unix_domain/unix_domain_server.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":unixdomain_client",
        "//score/datarouter/dynamic_configuration/i_session",
        "@score_baselibs//score/os:pthread",
        "@score_baselibs//score/os:sys_poll",
    ],
)

cc_library(
    name = "unixdomain_mock",
    hdrs = [
        "mocks/unix_domain/unix_domain_client.h",
        "mocks/unix_domain/unix_domain_server.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "mocks",
    visibility = [
        "//score/datarouter/dynamic_configuration:__subpackages__",
        "//score/datarouter/test:__subpackages__",
    ],
    deps = [
        ":unixdomain_common",
        "//score/datarouter/dynamic_configuration/i_session",
        "@googletest//:gtest_main",
    ],
)

## ===========================================================================
## Log parser
## ---------------------------------------------------------------------------

cc_library(
    name = "logparser_interface",
    hdrs = [
        "include/logparser/i_logparser.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//score/datarouter/file_transfer:__subpackages__",
        "//score/datarouter/nonverbose_dlt:__pkg__",
        "//score/datarouter/nonverbose_dlt_stub:__pkg__",
        "//score/datarouter/persistent_logging:__pkg__",
        "//score/datarouter/persistent_logging/persistent_logging:__pkg__",
        "//score/datarouter/test:__subpackages__",
    ],
    deps = [
        "//score/mw/log/detail/data_router/shared_memory:reader",
    ],
)

cc_library(
    name = "logparser",
    srcs = [
        "src/logparser/logparser.cpp",
    ],
    hdrs = [
        "include/logparser/logparser.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//score/datarouter/file_transfer:__subpackages__",
        "//score/datarouter/nonverbose_dlt:__pkg__",
        "//score/datarouter/nonverbose_dlt_stub:__pkg__",
        "//score/datarouter/persistent_logging:__pkg__",
        "//score/datarouter/persistent_logging/persistent_logging:__pkg__",
    ],
    deps = [
        ":datarouter_types",
        ":dltprotocol",
        ":log",
        ":logparser_interface",
        "//score/mw/log/detail/data_router/shared_memory:reader",
        "@score_baselibs//score/static_reflection_with_serialization/serialization",
    ],
)

cc_library(
    name = "logparser_testing",
    srcs = ["src/logparser/logparser.cpp"],
    hdrs = [
        "include/logparser/logparser.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":datarouter_types",
        ":dltprotocol",
        ":logparser_interface",
        "//score/mw/log/detail/data_router/shared_memory:reader",
        "@score_baselibs//score/mw/log/configuration:nvconfig",
        "@score_baselibs//score/static_reflection_with_serialization/serialization",
    ],
)

cc_library(
    name = "logparser_mock",
    hdrs = [
        "mocks/logparser_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "mocks",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":logparser_interface",
        "@googletest//:gtest_main",
    ],
)

## ===========================================================================
## libtracing
## ---------------------------------------------------------------------------

cc_library(
    name = "libdrconfighdrs",
    hdrs = ["include/daemon/data_router_cfg.h"],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include/daemon",
    visibility = [
        "//platform/aas/pas/datarouterconf:__subpackages__",
    ],
)

cc_library(
    name = "libtracing_legacy_public_headers",
    hdrs = [
        "include/Tracing",
        "include/logger/logger.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    tags = ["FFI"],
    visibility = [
        "//score/mw/log:__subpackages__",
        "@score_baselibs//score/mw/log:__subpackages__",
    ],
)

alias(
    name = "libtracing",
    actual = ":log",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "log",
    visibility = ["//visibility:public"],
    deps = [
        "//score/mw/log/detail/common:recorder_factory",
        "@score_baselibs//score/mw/log:frontend",
    ],
)

## ===========================================================================
##  Datarouter
## ---------------------------------------------------------------------------

cc_library(
    name = "datarouter_types",
    hdrs = [
        "include/router/data_router_types.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "@score_baselibs//score/ipctracing:__pkg__",
    ],
    deps = [
        ":dltprotocol",
        "@score_baselibs//score/static_reflection_with_serialization/visitor",
    ],
)

cc_library(
    name = "libstructdatasource",
    hdrs = ["include/StructDataSource"],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//visibility:private"],
    deps = [
        ":libdrconfighdrs",
    ],
)

## ===========================================================================
##  Datarouter
## ---------------------------------------------------------------------------

cc_library(
    name = "configurator_commands",
    hdrs = [
        "include/daemon/configurator_commands.h",
    ],
    visibility = [
        "//platform/aas/pas/datarouterconf:__subpackages__",
        "//score/datarouter:__subpackages__",
    ],
)

filegroup(
    name = "common_config_files",
    srcs = [
        "etc/key_value_storage_list.json",
        "etc/logging.json",
        "etc/persistent-database-override.json",
        "etc/persistent-logging.json",
    ],
    visibility = [
        "//ecu/xyz/xyz-shared/config/common/pas/datarouter:__subpackages__",
        "//score/datarouter/test:__subpackages__",
        "//platform/aas/tools/itf:__subpackages__",
        "//platform/aas/tools/sctf:__subpackages__",
        # "@ddad//ecu/xyz/xyz-shared/config/common/pas/datarouter:__subpackages__",
    ],
)

cc_library(
    name = "datarouter_options",
    srcs = [
        "src/applications/options.cpp",
    ],
    hdrs = [
        "src/applications/options.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "src/applications",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":log",
    ],
)

cc_library(
    name = "logchannel_utility",
    srcs = [
        "src/daemon/utility.cpp",
    ],
    hdrs = [
        "include/daemon/utility.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter:__subpackages__",
    ],
    deps = [
        ":log",
    ],
)

cc_library(
    name = "datarouter_lib",
    srcs = [
        "datarouter/data_router.cpp",
    ],
    hdrs = [
        "datarouter/data_router.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//visibility:private"],
    deps = [
        ":datarouter_types",
        ":log",
        ":logparser",
        ":message_passing_server",
        ":unixdomain_server",
        "//score/datarouter/lib/synchronized:synchronized_utility",
        "//score/mw/log/detail/data_router/shared_memory:reader",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "datarouter_benchmarking",
    srcs = [
        "datarouter/data_router.cpp",
    ],
    hdrs = [
        "datarouter/data_router.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/benchmark:__subpackages__",
    ],
    deps = [
        ":log",
        ":logparser",
        ":message_passing_server",
        ":unixdomain_mock",
        "//score/datarouter/lib/synchronized:synchronized_utility",
    ],
)

cc_library(
    name = "datarouter_testing",
    srcs = [
        "datarouter/data_router.cpp",
    ],
    hdrs = [
        "datarouter/data_router.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter:__subpackages__",
    ],
    deps = [
        ":datarouter_types",
        ":log",
        ":logparser_testing",
        ":message_passing_server",
        ":unixdomain_mock",
        "//score/datarouter/lib/synchronized:synchronized_utility",
    ],
)

cc_library(
    name = "dltserver_common",
    hdrs = [
        "include/daemon/dlt_log_server_config.h",
        "include/daemon/dltserver_common.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//score/datarouter/file_transfer:__subpackages__",
    ],
    deps = [
        ":dltprotocol",
        ":logparser",
    ],
)

cc_library(
    name = "log_sender",
    hdrs = [
        "include/daemon/i_log_sender.h",
        "include/daemon/log_sender.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//visibility:private"],
    deps = [
        ":dltprotocol",
        ":logparser",
    ],
)

[
    cc_library(
        name = "datarouter_feature_config" + name,
        testonly = test_only,
        hdrs = [
            "include/applications/datarouter_feature_config.h",
        ],
        defines = select({
            "//score/datarouter/build_configuration_flags:config_persistent_configuration": ["PERSISTENT_CONFIG_FEATURE_ENABLED"],
            "//conditions:default": [],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_datarouter_nonverbose_dlt": [
                "NON_VERBOSE_DLT",
            ],
            "//conditions:default": [],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_datarouter_dynamic_configuration": [
                "DYNAMIC_CONFIGURATION_IN_DATAROUTER",
            ],
            "//conditions:default": [],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_dlt_file_transfer": ["DLT_FILE_TRANSFER_FEATURE"],
            "//conditions:default": [],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_persistent_logging": ["PERSISTENT_LOGGING"],
            "//conditions:default": [],
        }),
        strip_include_prefix = "include",
        visibility = [
            "//score/datarouter:__subpackages__",
        ],
        deps = select({
            "//score/datarouter/build_configuration_flags:config_persistent_configuration": [
                ara_fact_dep,
            ],
            "//conditions:default": ["//score/datarouter/src/persistency/stub_persistent_dictionary:stub_persistent_dictionary_factory"],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_dlt_file_transfer": [
                "//score/datarouter/file_transfer/file_transfer_impl:file_transfer_stream_handler",
            ],
            "//conditions:default": [
                "//score/datarouter/file_transfer/file_transfer_stub:file_transfer_stream_handler_stub",
            ],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_datarouter_nonverbose_dlt": [
                nv_dep,
            ],
            "//conditions:default": [
                nv_stub_dep,
            ],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_datarouter_dynamic_configuration": [
                dc_dep,
            ],
            "//conditions:default": [
                dc_stub_dep,
            ],
        }) + select({
            "//score/datarouter/build_configuration_flags:config_persistent_logging": [
                "//score/datarouter/persistent_logging/persistent_logging_stub:sysedr_stub",
            ],
            "//conditions:default": [
                "//score/datarouter/persistent_logging/persistent_logging_stub:sysedr_stub",
            ],
        }),
    )
    for name, ara_fact_dep, nv_dep, nv_stub_dep, dc_dep, dc_stub_dep, test_only in [
        (
            "",
            "//score/datarouter/src/persistency/ara_per_persistent_dictionary:ara_per_persistent_dictionary_factory",
            "//score/datarouter/nonverbose_dlt:nonverbose_dlt_handler",
            "//score/datarouter/nonverbose_dlt_stub:nonverbose_dlt_handler_stub",
            "//score/datarouter/dynamic_configuration/dynamic_config_session:dynamic_config_session",
            "//score/datarouter/dynamic_configuration/config_session_stub:config_session_stub",
            False,
        ),
        (
            "_testing",
            "//score/datarouter/src/persistency/ara_per_persistent_dictionary:ara_per_persistent_dictionary_factory_testing",
            "//score/datarouter/nonverbose_dlt:nonverbose_dlt_handler_testing",
            "//score/datarouter/nonverbose_dlt_stub:nonverbose_dlt_handler_stub_testing",
            "//score/datarouter/dynamic_configuration/dynamic_config_session:dynamic_config_session_testing",
            "//score/datarouter/dynamic_configuration/config_session_stub:config_session_stub_testing",
            True,
        ),
    ]
]

cc_library(
    name = "dltserver",
    srcs = [
        "src/daemon/diagnostic_job_handler.cpp",
        "src/daemon/diagnostic_job_parser.cpp",
        "src/daemon/dlt_log_server.cpp",
        "src/daemon/verbose_dlt.cpp",
    ],
    hdrs = [
        "include/daemon/diagnostic_job_handler.h",
        "include/daemon/diagnostic_job_parser.h",
        "include/daemon/dlt_log_server.h",
        "include/daemon/i_diagnostic_job_handler.h",
        "include/daemon/i_diagnostic_job_parser.h",
        "include/daemon/i_dlt_log_server.h",
        "include/daemon/verbose_dlt.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//visibility:private"],
    deps = [
        ":configurator_commands",
        ":datarouter_feature_config",
        ":dlt_log_channel_lib",
        ":dltserver_common",
        ":libdrconfighdrs",
        ":libtracing",
        ":log",
        ":log_entry_deserialization",
        ":log_sender",
        ":logchannel_utility",
        ":udp_stream_output",
        ":unixdomain_server",
        "//score/datarouter/network:vlan",
        "//score/datarouter/persistent_logging/persistent_logging_stub:sysedr_stub",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log/configuration:nvconfig",
        "@score_baselibs//score/os:socket",
        "@score_baselibs//score/os:stat",
        "@score_baselibs//score/os:stdio",
        "@score_baselibs//score/os:unistd",
        "@score_baselibs//score/os/utils:thread",
    ],
)

[
    cc_library(
        name = "dlt_log_channel_lib" + name,
        testonly = test_only,
        srcs = [
            "src/daemon/dlt_log_channel.cpp",
        ],
        hdrs = [
            "include/daemon/dlt_log_channel.h",
        ],
        features = COMPILER_WARNING_FEATURES,
        strip_include_prefix = "include",
        visibility = [
            "//score/datarouter/nonverbose_dlt:__pkg__",
            "//score/datarouter/nonverbose_dlt_stub:__pkg__",
        ],
        deps = [
            ":dltserver_common",
            ":log",
            "@score_baselibs//score/mw/log/configuration:nvconfig",
            "@score_baselibs//score/os:socket",
            "@score_baselibs//score/os:stat",
            "@score_baselibs//score/os:stdio",
            "@score_baselibs//score/os:unistd",
            "@score_baselibs//score/os/utils:thread",
        ] + dep,
    )
    for name, dep, test_only in [
        (
            "",
            [
                ":logparser",
                ":udp_stream_output",
            ],
            False,
        ),
        (
            "_testing",
            [
                ":logparser_testing",
                ":udpoutput_mock",
            ],
            True,
        ),
    ]
]

cc_library(
    name = "socketserver_config_helpers",
    hdrs = [
        "include/daemon/socketserver_json_helpers.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//visibility:private"],
    deps = [
        "@rapidjson",
    ],
)

cc_library(
    name = "persistentlogconfig",
    srcs = [
        "src/daemon/persistentlogging_config.cpp",
    ],
    hdrs = [
        "include/daemon/persistentlogging_config.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//score/datarouter:__subpackages__",
    ],
    deps = [
        ":dltprotocol",
        ":log",
        ":logchannel_utility",
        ":socketserver_config_helpers",
        "//score/datarouter/src/persistency:interface",
        "@rapidjson",
    ],
)

cc_library(
    name = "socketserver_config_lib",
    srcs = [
        "src/daemon/socketserver_config.cpp",
        "src/daemon/socketserver_filter_factory.cpp",
    ],
    hdrs = [
        "include/daemon/socketserver_config.h",
        "include/daemon/socketserver_filter_factory.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//visibility:private"],
    deps = [
        ":datarouter_feature_config",
        ":datarouter_lib",
        ":dltserver_common",
        ":persistentlogconfig",
        ":socketserver_config_helpers",
        "//score/datarouter:datarouter_types",
        "//score/datarouter/error:logging_error",
        "//score/datarouter/src/persistency:interface",
        "@rapidjson",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/static_reflection_with_serialization/serialization",
        "@score_baselibs//score/static_reflection_with_serialization/visitor",
    ],
)

cc_library(
    name = "socketserver_config_lib_testing",
    testonly = True,
    srcs = [
        "src/daemon/socketserver_config.cpp",
        "src/daemon/socketserver_filter_factory.cpp",
    ],
    hdrs = [
        "include/daemon/socketserver_config.h",
        "include/daemon/socketserver_filter_factory.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":datarouter_feature_config",
        ":datarouter_testing",
        ":dltserver_common",
        ":persistentlogconfig",
        "//score/datarouter:datarouter_types",
        "//score/datarouter/error:logging_error",
        "//score/datarouter/src/persistency:mock",
        "@rapidjson",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/static_reflection_with_serialization/serialization",
        "@score_baselibs//score/static_reflection_with_serialization/visitor",
    ],
)

cc_library(
    name = "datarouter_socketserver",
    srcs = [
        "src/daemon/socketserver.cpp",
    ],
    hdrs = [
        "include/daemon/socketserver.h",
    ],
    # TODO: will be reworked in Ticket-207823
    features = COMPILER_WARNING_FEATURES,
    local_defines = select({
        "//score/datarouter/build_configuration_flags:config_persistent_logging": ["PERSISTENT_LOGGING"],
        "//conditions:default": [],
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:private"],
    deps = [
        ":datarouter_feature_config",
        ":datarouter_lib",
        ":dltserver",
        ":persistentlogconfig",
        ":socketserver_config_lib",
        "@score_baselibs//score/mw/log/configuration:nvconfigfactory",
    ] + select({
        "//score/datarouter/build_configuration_flags:config_persistent_logging": [
            "//score/datarouter/persistent_logging/persistent_logging_stub:sysedr_stub",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "datarouter_socketserver_testing",
    testonly = True,
    srcs = [
        "src/daemon/socketserver.cpp",
    ],
    hdrs = [
        "include/daemon/socketserver.h",
    ],
    # TODO: will be reworked in Ticket-207823
    features = COMPILER_WARNING_FEATURES,
    local_defines = select({
        "//score/datarouter/build_configuration_flags:config_persistent_logging": ["PERSISTENT_LOGGING"],
        "//conditions:default": [],
    }),
    strip_include_prefix = "include",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":datarouter_feature_config",
        ":datarouter_lib",
        ":dltserver",
        ":persistentlogconfig",
        ":socketserver_config_lib_testing",
        "@score_baselibs//score/mw/log/configuration:nvconfigfactory",
    ] + select({
        "//score/datarouter/build_configuration_flags:config_persistent_logging": [
            "//score/datarouter/persistent_logging/persistent_logging_stub:sysedr_stub",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "datarouter_app",
    srcs = [
        "src/applications/datarouter_app.cpp",
    ],
    hdrs = [
        "src/applications/datarouter_app.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "src/applications",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":datarouter_options",
        ":datarouter_socketserver",
        ":log",
    ],
)

cc_binary(
    name = "datarouter",
    srcs = [
        "src/applications/main_nonadaptive.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":datarouter_app",
        "@score_baselibs//score/os:pthread",
        "@score_baselibs//score/os/utils:path",
        # "//third_party/jemalloc", # Ticket-231781
    ],
)

cc_library(
    name = "message_passing_server",
    srcs = [
        "src/daemon/message_passing_server.cpp",
    ],
    hdrs = [
        "include/daemon/message_passing_server.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        "//score/datarouter/daemon_communication:daemon_session_handle_interface",
        "//score/mw/log/detail/data_router:message_passing_interface",
        "//score/mw/log/detail/data_router/shared_memory:reader",
        "@score_baselibs//score/os:pthread",
        "@score_communication//score/mw/com/message_passing",
    ],
)

cc_library(
    name = "udp_send_interface",
    hdrs = [
        "include/daemon/i_udp_send.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "udp_stream_output",
    srcs = [
        "src/daemon/udp_stream_output.cpp",
    ],
    hdrs = [
        "include/daemon/udp_stream_output.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = ["//score/datarouter:__subpackages__"],
    deps = [
        ":dltserver_common",
        "//score/datarouter/network:vlan",
        "@score_baselibs//score/os:pthread",
        "@score_baselibs//score/os:socket",
        "@score_baselibs//score/os:unistd",
    ],
)

cc_library(
    name = "udpoutput_mock",
    hdrs = [
        "mocks/daemon/udp_stream_output.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "mocks",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":dltserver_common",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "log_sender_mock",
    hdrs = [
        "mocks/daemon/log_sender_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "mocks",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":dltserver_common",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "diagnostic_job_handler_mock",
    hdrs = [
        "mocks/daemon/diagnostic_job_handler_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "mocks",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":dltserver_common",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "diagnostic_job_parser_mock",
    hdrs = [
        "mocks/daemon/diagnostic_job_parser_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "mocks",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":dltserver_common",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "dlt_log_server_mock",
    hdrs = [
        "mocks/daemon/dlt_log_server_mock.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "mocks",
    visibility = ["//score/datarouter/test:__subpackages__"],
    deps = [
        ":dltserver_common",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "log_entry_deserialization",
    srcs = [
        "src/daemon/log_entry_deserialization_visitor.cpp",
    ],
    hdrs = [
        "include/daemon/log_entry_deserialization_visitor.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test:__subpackages__",
    ],
    deps = [
        ":configurator_commands",
        "//score/mw/log/detail/common:log_entry_deserialize",
        "@score_baselibs//score/static_reflection_with_serialization/serialization",
    ],
)

cc_library(
    name = "dltserver_testing",
    testonly = True,
    srcs = [
        "src/daemon/diagnostic_job_handler.cpp",
        "src/daemon/diagnostic_job_parser.cpp",
        "src/daemon/dlt_log_channel.cpp",
        "src/daemon/dlt_log_server.cpp",
        "src/daemon/verbose_dlt.cpp",
    ],
    hdrs = [
        "include/daemon/diagnostic_job_handler.h",
        "include/daemon/diagnostic_job_parser.h",
        "include/daemon/dlt_log_channel.h",
        "include/daemon/dlt_log_server.h",
        "include/daemon/dltserver_common.h",
        "include/daemon/i_diagnostic_job_handler.h",
        "include/daemon/i_diagnostic_job_parser.h",
        "include/daemon/i_dlt_log_server.h",
        "include/daemon/i_log_sender.h",
        "include/daemon/log_sender.h",
        "include/daemon/verbose_dlt.h",
    ],
    features = COMPILER_WARNING_FEATURES,
    strip_include_prefix = "include",
    visibility = [
        "//score/datarouter:__subpackages__",
    ],
    deps = [
        # It's being conflicted with the real udp_stream_output target that used
        # by earlier targets here. "udpoutput_mock" shall be at the first.
        # buildifier off
        ":udpoutput_mock",
        # buildifier on
        ":unixdomain_mock",
        ":datarouter_feature_config",
        ":datarouter_feature_config_testing",
        ":datarouter_types",
        ":diagnostic_job_handler_mock",
        ":diagnostic_job_parser_mock",
        ":dlt_log_server_mock",
        ":log_entry_deserialization",
        ":log_sender_mock",
        ":logparser_testing",
        ":configurator_commands",
        "@score_baselibs//score/os:socket",
        "@score_baselibs//score/os:stat",
        "@score_baselibs//score/os:stdio",
        "@score_baselibs//score/os/utils:path",
        "@score_baselibs//score/os/utils:thread",
        ":log",
        "@score_baselibs//score/mw/log/configuration:nvconfig",
        "//score/datarouter/dlt_filetransfer_trigger_lib:filetransfer_message_types",
        "//score/datarouter/persistent_logging/persistent_logging_stub:sysedr_stub",
    ],
)

## ===========================================================================
##  Unit tests
## ---------------------------------------------------------------------------

test_suite(
    name = "unit_tests",
    tests = [
        #"//score/datarouter/persistent_log_request:unit_tests",
        #"//score/datarouter/test:unit_tests",
        #"//score/datarouter/tools/generator:unit_tests",
    ],
    visibility = ["//platform/aas/pas:__pkg__"],
)

exports_files(
    ["common_build_variables.bzl"],
    visibility = ["//visibility:public"],
)
