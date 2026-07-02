load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@score_baselibs//score/language/safecpp:toolchain_features.bzl", "COMPILER_WARNING_FEATURES")

package(default_testonly = True)

cc_binary(
    name = "cross_locking_app",
    srcs = [
        "src/cross_locking_app.cpp",
    ],
    features = COMPILER_WARNING_FEATURES,
    visibility = [
        "//score/datarouter/test/itf:__pkg__",
    ],
    deps = [
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
