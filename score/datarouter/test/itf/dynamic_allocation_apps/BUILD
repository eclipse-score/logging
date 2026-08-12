load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

package(default_testonly = True)

cc_binary(
    name = "ara_log_golden_app",
    srcs = [
        "src/ara_log_golden_app.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
    deps = [
        "//platform/aas/test/pas/perf_tests/tools/malloc_profiler:malloc-profiler",
        "@score_baselibs//score/mw/log",
    ],
)

cc_binary(
    name = "ara_log_allocations",
    srcs = [
        "src/ara_log_allocations.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
    deps = [
        "//platform/aas/ara/core",
        "//platform/aas/test/pas/perf_tests/tools/malloc_profiler:malloc-profiler",
        "@score_baselibs//score/mw/log",
    ],
)

cc_binary(
    name = "non_verbose_allocations",
    srcs = [
        "src/non_verbose_allocations.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
    deps = [
        "//platform/aas/test/pas/perf_tests/tools/malloc_profiler:malloc-profiler",
        "//score/mw/log/legacy_non_verbose_api",
        "@score_baselibs//score/mw/log",
    ],
)

cc_binary(
    name = "non_verbose_golden_app",
    srcs = [
        "src/non_verbose_golden_app.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
    deps = [
        "//platform/aas/test/pas/perf_tests/tools/malloc_profiler:malloc-profiler",
        "//score/mw/log/legacy_non_verbose_api",
        "@score_baselibs//score/mw/log",
    ],
)

cc_binary(
    name = "mw_log_golden_app",
    srcs = [
        "src/mw_log_golden_app.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
    deps = [
        "//platform/aas/test/pas/perf_tests/tools/malloc_profiler:malloc-profiler",
        "@score_baselibs//score/mw/log",
    ],
)

cc_binary(
    name = "mw_log_allocations",
    srcs = [
        "src/mw_log_allocations.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
    deps = [
        "//platform/aas/test/pas/perf_tests/tools/malloc_profiler:malloc-profiler",
        "@score_baselibs//score/mw/log",
    ],
)

filegroup(
    name = "logging_config",
    testonly = True,
    srcs = [
        "etc/logging.json",
    ],
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
)
