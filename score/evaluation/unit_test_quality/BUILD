
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "unit_test_quality",
    hdrs = ["unit_test_quality.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "unit_test_quality_test",
    srcs = ["unit_test_quality_test.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":unit_test_quality",
        "@googletest//:gtest_main",
    ],
)
