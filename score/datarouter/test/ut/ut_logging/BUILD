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

load("@score_baselibs//third_party/itf:py_unittest_qnx_test.bzl", "py_unittest_qnx_test")

FEAT_COMPILER_WARNINGS_AS_ERRORS = [
    "treat_warnings_as_errors",
    "strict_warnings",
    "additional_warnings",
]

## ---------------------------------------------------------------------------
## Unit tests
## ---------------------------------------------------------------------------

test_suite(
    name = "unit_tests",
    tests = [
        ":FileTransferHandlerFactoryUT",
        ":datarouterAppUT",
        ":dlt_verbose_handler_test",
        ":dltprotocolUT",
        ":dltserverUT",
        ":errorUT",
        ":filetransferUT",
        ":log_entry_deserialize_test",
        ":logparserUT",
        ":messagePassingServerUT",
        ":persistentLogConfigUT",
        ":socketserverConfigUT",
        ":socketserverUT",
        ":udp_stream_output_test",
        ":unix_domain_server_test",
        ":utility_test",
    ],
    visibility = ["//score/datarouter:__pkg__"],
)

cc_test(
    name = "log_entry_deserialize_test",
    srcs = [
        "test_log_entry_deserialize.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    includes = [
        "include",
        "src",
        "ut",
    ],
    tags = ["unit"],
    deps = [
        "//score/datarouter:log_entry_deserialization",
        "@googletest//:gtest_main",
        "@score_baselibs//score/static_reflection_with_serialization/serialization",
    ],
)

cc_test(
    name = "utility_test",
    srcs = [
        "utility_test.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    includes = [
        "include",
        "src",
        "ut",
    ],
    tags = ["unit"],
    deps = [
        "//score/datarouter:logchannel_utility",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "logparserUT",
    srcs = [
        "test_logparser.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:logparser_testing",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log/configuration:nvconfig_mock",
    ],
)

cc_test(
    name = "errorUT",
    srcs = [
        "test_error.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter/error:logging_error",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dltprotocolUT",
    srcs = [
        "test_dltprotocol.cpp",
    ],
    data = [
        "//score/datarouter/test/ut/etc:fg_dummy_file_transfer",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:dltprotocol",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "filetransferUT",
    srcs = [
        "filetransferTest.cpp",
    ],
    defines = select({
        "//score/datarouter/build_configuration_flags:config_file_transfer": ["DLT_FILE_TRANSFER_FEATURE"],
        "//conditions:default": [],
    }),
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter/dlt_filetransfer_trigger_lib",
        "//score/datarouter/dlt_filetransfer_trigger_lib:filetransfer_message_types",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "FileTransferHandlerFactoryUT",
    srcs = ["test_file_transfer_handler_factory.cpp"],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:datarouter_feature_config",
        "//score/datarouter/src/file_transfer:file_transfer_handler_factory",
        "@googletest//:gtest_main",
    ] + select({
        "//score/datarouter/build_configuration_flags:config_file_transfer": [
            "//score/datarouter/src/file_transfer/file_transfer_impl:file_transfer_stream_handler_factory",
        ],
        "//conditions:default": [
            "//score/datarouter/src/file_transfer/file_transfer_stub:file_transfer_handler_factory_stub",
        ],
    }),
)

cc_test(
    name = "PersistentDictionaryFactoryUT",
    srcs = select({
        "//score/datarouter/build_configuration_flags:config_persistent_configuration": [
            "test_persistent_dictionary_factory.cpp",
            "test_stub_persistent_dictionary_factory.cpp",
        ],
        "//conditions:default": [
            "test_stub_persistent_dictionary_factory.cpp",
        ],
    }),
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = select({
        "//score/datarouter/build_configuration_flags:config_persistent_configuration": [
            "//score/datarouter/src/persistency/ara_per_persistent_dictionary:ara_per_persistent_dictionary_factory_testing",
        ],
        "//conditions:default": [
            "//score/datarouter/src/persistency/stub_persistent_dictionary:stub_persistent_dictionary_factory",
        ],
    }) + [
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dlt_verbose_handler_test",
    srcs = [
        "test_verbose_dlt.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:dltserver_testing",
        "//score/datarouter:log",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dltserverUT",
    srcs = [
        "test_diagnostic_job_handler.cpp",
        "test_diagnostic_job_parser.cpp",
        "test_dltchannel.cpp",
        "test_dltserver.cpp",
    ] + select({
        "//score/datarouter/build_configuration_flags:config_datarouter_nonverbose_dlt": [
            "test_nonverbosedlt.cpp",
        ],
        "//conditions:default": [],
    }) + select({
        "//score/datarouter/build_configuration_flags:config_file_transfer": ["test_filetransfer_stream.cpp"],
        "//conditions:default": [],
    }),
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        # It's being conflicted with the real udp_stream_output target that used
        # by earlier targets here. "dltserver_testing" shall be at the first.
        # buildifier off
        "//score/datarouter:dltserver_testing",
        # buildifier on
        "//score/datarouter:log",
        "//score/datarouter:datarouter_feature_config_testing",
        "//score/datarouter/dlt_filetransfer_trigger_lib",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "optionsUT",
    srcs = [
        "test_options.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:datarouter_options",
        "//score/datarouter:log",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "datarouterAppUT",
    srcs = [
        "test_datarouter_app.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:datarouter_app",
        "//score/datarouter:log",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "persistentLogConfigUT",
    srcs = [
        "test_persistentlog_config.cpp",
    ],
    data = [
        "//score/datarouter/test/ut/etc:fg_persistent_logging_test_json",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:log",
        "//score/datarouter:persistentlogconfig",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "socketserverConfigUT",
    srcs = [
        "test_socketserver_config.cpp",
    ],
    data = [
        "//score/datarouter/test/ut/etc:fg_socket_server_log_channels_test_json",
        "//score/datarouter/test/ut/etc:fg_socketserver_config_test_json",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:log",
        "//score/datarouter:socketserver_config_lib_testing",
        "//score/datarouter/src/persistency/stub_persistent_dictionary:stub",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "socketserverUT",
    srcs = [
        "test_socketserver.cpp",
    ],
    data = [
        "//score/datarouter/test/ut/etc:fg_socket_server_log_channels_test_json",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:datarouter_socketserver_testing",
        "//score/datarouter/src/persistency:mock",
        "@googletest//:gtest_main",
        "@score_baselibs//score/mw/log/configuration:nvconfig_mock",
        "@score_baselibs//score/os/mocklib:pthread_mock",
        "@score_baselibs//score/os/mocklib:unistd_mock",
    ],
)

cc_test(
    name = "messagePassingServerUT",
    srcs = [
        "test_message_passing_server.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:message_passing_server",
        "//score/datarouter/daemon_communication:mock",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:pthread_mock",
        "@score_baselibs//score/os/mocklib:unistd_mock",
        "@score_communication//score/message_passing:mock",
    ],
)

cc_test(
    name = "udp_stream_output_test",
    srcs = [
        "test_udp_stream_output.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:udp_stream_output",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:socket_mock",
    ],
)

cc_test(
    name = "unix_domain_server_test",
    srcs = [
        "test_unix_domain_server.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        "//score/datarouter:unixdomain_server",
        "@googletest//:gtest_main",
        "@score_baselibs//score/os/mocklib:pthread_mock",
        "@score_baselibs//score/os/mocklib:socket_mock",
        "@score_baselibs//score/os/mocklib:sys_poll_mock",
        "@score_baselibs//score/os/utils/mocklib:signal_mock",
    ],
)

cc_test(
    name = "stubConfigSessionUT",
    testonly = True,
    srcs = select({
        "//score/datarouter/build_configuration_flags:config_datarouter_dynamic_configuration": [],
        "//conditions:default": [
            "test_stub_config_session.cpp",
        ],
    }),
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = select({
        "//score/datarouter/build_configuration_flags:config_datarouter_dynamic_configuration": [
            "@googletest//:gtest_main",
        ],
        "//conditions:default": [
            "//score/datarouter:datarouter_feature_config_testing",
            "//score/datarouter/dynamic_configuration/config_session_stub:config_session_stub_testing",
            "@googletest//:gtest_main",
        ],
    }),
)

cc_test(
    name = "configSessionFactoryUT",
    testonly = True,
    srcs = [
        "test_config_session_factory.cpp",
    ],
    features = FEAT_COMPILER_WARNINGS_AS_ERRORS,
    tags = ["unit"],
    deps = [
        # It's being conflicted with the real udp_stream_output target that used
        # by earlier targets here. "udpoutput_mock" shall be at the first.
        # buildifier off
        "//score/datarouter:udpoutput_mock",
        # buildifier on
        "//score/datarouter:datarouter_feature_config_testing",
        "//score/datarouter:dltserver_testing",
        # "//score/datarouter/dynamic_configuration/dynamic_config_session:dynamic_config_session_testing",
        "//score/datarouter/dynamic_configuration/config_session_stub:config_session_stub_testing",
        "@googletest//:gtest_main",
    ],
)

py_unittest_qnx_test(
    name = "unit_tests_qnx",
    data_files = [
        "//score/datarouter/test/ut/etc:fg_persistent_logging_test_json",
        "//score/datarouter/test/ut/etc:fg_socketserver_config_test_json",
        "//score/datarouter/test/ut/etc:fg_socket_server_log_channels_test_json",
    ],
    test_cases = [
        ":errorUT",
        ":filetransferUT",
        ":datarouterAppUT",
        ":dltprotocolUT",
        ":optionsUT",
        ":dltserverUT",
        ":logparserUT",
        ":persistentLogConfigUT",
        ":dlt_verbose_handler_test",
        ":udp_stream_output_test",
        ":unix_domain_server_test",
        ":messagePassingServerUT",
        ":socketserverConfigUT",
        ":socketserverUT",
        ":log_entry_deserialize_test",
        ":utility_test",
        ":FileTransferHandlerFactoryUT",
        ":PersistentDictionaryFactoryUT",
        "//score/datarouter/persistent_log_request/test:test_persistent_log_request",
        "//score/datarouter/tools/generator:fibex_generator_test",
    ],
    visibility = ["//visibility:public"],  # platform_only
)
