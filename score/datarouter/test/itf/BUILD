load("@rules_python//python:defs.bzl", "py_binary")
load("@score_logging//third_party/itf:py_unittest_qnx_test.bzl", "py_itf_test")

package(default_testonly = True)

py_itf_test(
    name = "valgrind_check",
    srcs = [
        "main.py",
        "test_valgrind.py",
    ],
    data = [
        "//score/datarouter/test/itf/dynamic_allocation_apps:logging_config",
        "//score/datarouter/test/itf/logging_cross_locking_app:logging_config",
    ],
    data_target = [
        "//platform/aas/tools/valgrind/qnx:valgrind-pkg",
        "//platform/aas/tools/valgrind/qnx:valgrind-supp-files",
        "//score/datarouter/test/itf/dynamic_allocation_apps:ara_log_allocations",
        "//score/datarouter/test/itf/logging_cross_locking_app:cross_locking_app",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    visibility = [
        "//platform/aas/test:__pkg__",
    ],
    deps = [
        "//platform/aas/test/pas:pas_common",
        "//platform/aas/tools/valgrind:valgrind_tools",
    ],
)

py_itf_test(
    name = "integration_tests",
    srcs = [
        "main.py",
    ],
    extra_tags = ["itf_bat"],
    max_failures = 100,
    module_root = "platform/aas/pas/logging/test/itf",
    test_suites = [
        ":dr_restart",
        ":filetransfer",
        ":file_logging_test",
        ":logging_console",
        ":logging_smoke",
        ":test_safe_logging_ipc",
        ":test_logging_allocation",
        ":test_logging_cross_locking",
        ":test_bandwidth_quotes_exists",
        ":test_change_loglevel",
        ":test_quota_exceed",
        ":test_ecu_modes_switching",
        ":test_logging_detached_logs",
        ":test_logging_after_delayed_dr_start",
        ":test_dr_session_memory_leak",
        "test_sender_timeout",
    ],
    visibility = ["//platform/aas/test:__pkg__"],
)

py_itf_test(
    name = "filetransfer",
    srcs = [
        "main.py",
        "test_filetransfer.py",
    ],
    data_target = [
        ":filetransfer-app-artifacts",
        "//score/datarouter/test/sct/filetransfer_over_dlt:fg_filetransfer_testfiles",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
)

genrule(
    name = "filetransfer-app-artifacts",
    testonly = 1,
    srcs = [
        "//platform/aas/test/pas/filetransfer_application:filetransfer_app",
        "//platform/tools/extract_files_frm_dlt",
    ],
    outs = [
        "filetransfer_app",
        "extract_files_frm_dlt",
        "extract_files_frm_dlt.py",
    ],
    cmd = """
            cp $(location //platform/aas/test/pas/filetransfer_application:filetransfer_app) $(@D)
            cp $(locations //platform/tools/extract_files_frm_dlt:extract_files_frm_dlt) $(@D)
          """,
    output_to_bindir = True,
)

py_itf_test(
    name = "bat_tests",
    srcs = [
        "main.py",
    ],
    extra_tags = ["itf_bat"],
    max_failures = 100,
    module_root = "platform/aas/pas/logging/test/itf",
    test_suites = [
        ":logging_smoke",
    ],
    visibility = [
        "//platform/aas/test:__pkg__",
        "//platform/aas/test/pas:__pkg__",
    ],
)

py_itf_test(
    name = "file_logging_test",
    srcs = [
        "main.py",
        "test_file_logging.py",
    ],
    data_target = [
        ":logging-app-artifacts",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [
        "//platform/aas/test/pas:machine_state",
        "//platform/aas/test/pas:pas_common",
    ],
)

py_itf_test(
    name = "logging_smoke",
    srcs = [
        "logging_diagnostics.py",
        "main.py",
        "test_logging_smoke.py",
    ],
    data = [
        "//pqr/abc/abc-lmn/config/project/cpu/pas/logging:log_channel_config_test",
    ],
    data_target = [
        ":logging-app-artifacts",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [
        "//platform/aas/pas/std_frame/test/sct:stdframe_messageparser_lib",
        "//platform/aas/test/pas:machine_state",
        "//platform/aas/test/pas:pas_common",
        "//platform/aas/tools/perf_report",
    ],
)

py_itf_test(
    name = "logging_console",
    srcs = [
        "main.py",
        "test_logging_console.py",
    ],
    data_target = [
        ":logging-app-artifacts",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [
        "//platform/aas/test/pas:pas_common",
        "//platform/aas/tools/perf_report",
    ],
)

genrule(
    name = "logging-app-artifacts",
    testonly = 1,
    srcs = [
        "//score/mw/log/test/dlt_generator",
    ],
    outs = [
        "dlt_generator",
    ],
    cmd = """
            cp $(location @score_logging//score/mw/log/test/dlt_generator:dlt_generator) $(@D)
          """,
    output_to_bindir = True,
)

py_itf_test(
    name = "dr_restart",
    srcs = [
        "main.py",
        "test_logging_dr_restart.py",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
)

py_binary(
    name = "dlt_quality_of_service",
    srcs = [
        "dlt_quality_of_service.py",
    ],
    data = [
        "//platform/aas/tools/python_dlt:dlt-artifacts",
        "//third_party/dltdump",
    ],
    deps = [
        "//platform/aas/tools/python_dlt",
    ],
)

py_itf_test(
    name = "quality_of_service",
    srcs = [
        "main.py",
        "test_quality_of_service.py",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [":dlt_quality_of_service"],
)

####################Safe IPC###############################

genrule(
    name = "fault-app-artifacts",
    testonly = 1,
    srcs = [
        "//score/datarouter/test/itf/safe_ipc_fault_app:fault_app",
    ],
    outs = [
        "fault_app",
    ],
    cmd = """
            cp $(location //score/datarouter/test/itf/safe_ipc_fault_app:fault_app) $(@D)
          """,
    output_to_bindir = True,
)

py_itf_test(
    name = "test_safe_logging_ipc",
    srcs = [
        "main.py",
        "test_safe_logging_ipc.py",
    ],
    data = [
        "//score/datarouter/test/itf/safe_ipc_fault_app:logging_config",
    ],
    data_target = [
        ":fault_app",
        ":logging-app-artifacts",
        ":fault-app-artifacts",
    ],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [
        "//platform/aas/test/pas:apps_started_fixture",
    ],
)

py_itf_test(
    name = "test_logging_allocation",
    srcs = [
        "main.py",
        "test_logging_allocation.py",
    ],
    data = [
        "//score/datarouter/test/itf/dynamic_allocation_apps:logging_config",
    ],
    data_target = [
        "//score/datarouter/test/itf/dynamic_allocation_apps:ara_log_golden_app",
        "//score/datarouter/test/itf/dynamic_allocation_apps:ara_log_allocations",
        "//score/datarouter/test/itf/dynamic_allocation_apps:mw_log_golden_app",
        "//score/datarouter/test/itf/dynamic_allocation_apps:mw_log_allocations",
        "//score/datarouter/test/itf/dynamic_allocation_apps:non_verbose_allocations",
        "//score/datarouter/test/itf/dynamic_allocation_apps:non_verbose_golden_app",
        "//platform/aas/test/pas/perf_tests/tools/malloc_profiler:malloc-profiler",
    ],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [
        "//platform/aas/test/pas:perf_test_dependency",
    ],
)

py_itf_test(
    name = "test_logging_cross_locking",
    srcs = [
        "main.py",
        "test_logging_cross_locking.py",
    ],
    data = [
        "//score/datarouter/test/itf/logging_cross_locking_app:logging_config",
        "//third_party/ipnext_qnx/sdp:traceprinter",
    ],
    data_target = [
        "//score/datarouter/test/itf/logging_cross_locking_app:cross_locking_app",
    ],
    module_root = "platform/aas/pas/logging/test/itf",
)

py_itf_test(
    name = "test_bandwidth_quotes_exists",
    srcs = [
        "main.py",
        "test_bandwidth_quotes_exists.py",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = ["//platform/aas/test/pas:pas_common"],
)

py_itf_test(
    name = "test_change_loglevel",
    srcs = [
        "main.py",
        "test_change_loglevel.py",
    ],
    data = [
        "//score/datarouter/test/itf/change_loglevel:change_log_level_test_logging_cfg",
    ],
    data_target = [
        "//score/datarouter/test/itf/change_loglevel:change_log_level_test_app",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    visibility = ["//platform/aas/test:__pkg__"],
    deps = [
        "//platform/aas/test/pas:pas_common",
    ],
)

py_itf_test(
    name = "test_quota_exceed",
    srcs = [
        "main.py",
        "test_quota_exceed.py",
    ],
    data = [
        "etc/LGGG_100_TRUE/log-channels.json",
        "etc/LGGG_500_FALSE/log-channels.json",
        "etc/LGGG_500_TRUE/log-channels.json",
    ],
    data_target = [
        ":logging-app-artifacts",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [
        "//platform/aas/test/pas:machine_state",
        "//platform/aas/test/pas:pas_common",
        "//platform/aas/tools/perf_report",
        "//platform/aas/tools/python_dlt",
    ],
)

py_itf_test(
    name = "test_ecu_modes_switching",
    srcs = [
        "conftest.py",
        "main.py",
        "sfa_token_helpers.py",
        "test_ecu_modes_switching.py",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    visibility = [
        "//platform/aas/test:__pkg__",
    ],
    deps = [
        "//platform/aas/tools/xtf_common",
    ],
)

py_itf_test(
    name = "test_logging_detached_logs",
    srcs = [
        "conftest.py",
        "main.py",
        "sfa_token_helpers.py",
        "test_logging_detached_logs.py",
    ],
    data_target = [
        ":logging-app-artifacts",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    visibility = [
        "//platform/aas/test:__pkg__",
    ],
    deps = [
        "//platform/aas/tools/xtf_common",
    ],
)

py_itf_test(
    name = "test_logging_after_delayed_dr_start",
    srcs = [
        "conftest.py",
        "main.py",
        "sfa_token_helpers.py",
        "test_logging_after_delayed_dr_start.py",
    ],
    data_target = [
        ":logging-app-artifacts",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    visibility = [
        "//platform/aas/test:__pkg__",
    ],
    deps = [
        "//platform/aas/tools/xtf_common",
    ],
)

py_itf_test(
    name = "test_dr_session_memory_leak",
    srcs = [
        "conftest.py",
        "main.py",
        "sfa_token_helpers.py",
        "test_dr_session_memory_leak.py",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    visibility = [
        "//platform/aas/test:__pkg__",
    ],
    deps = [
        "//platform/aas/tools/xtf_common",
    ],
)

py_itf_test(
    name = "test_sender_timeout",
    srcs = [
        "main.py",
        "test_sender_timeout.py",
    ],
    data_target = [
        ":logging-app-artifacts",
    ],
    extra_tags = ["itf_bat"],
    module_root = "platform/aas/pas/logging/test/itf",
    deps = [
        "//platform/aas/test/pas:pas_common",
    ],
)
