load("@rules_cc//cc:defs.bzl", "cc_binary")

package(default_testonly = True)

cc_binary(
    name = "fault_app",
    testonly = True,
    srcs = [
        "src/fault_app.cpp",
    ],
    visibility = ["//visibility:public"],
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
    visibility = ["//visibility:public"],
)
