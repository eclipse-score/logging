load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_python//python:defs.bzl", "py_library")
load("@score_itf//bazel:py_itf_plugin.bzl", "py_itf_plugin")

pkg_tar(
    name = "dlt_pkg",
    srcs = ["@score_itf//third_party/dlt:dlt-receive"],
    visibility = ["//score/test/integration:__subpackages__"],
)

pkg_tar(
    name = "datarouter_bin_pkg",
    srcs = ["//score/datarouter"],
    package_dir = "opt/datarouter/bin",
)

pkg_tar(
    name = "datarouter_conf_pkg",
    srcs = ["//score/datarouter:integration_config_files"],
    package_dir = "opt/datarouter",
    strip_prefix = "/score/datarouter",
)

pkg_tar(
    name = "datarouter_pkg",
    visibility = ["//score/test/integration:__subpackages__"],
    deps = [
        ":datarouter_bin_pkg",
        ":datarouter_conf_pkg",
    ],
)

py_library(
    name = "dlt_parser",
    srcs = ["dlt_parser.py"],
    imports = ["."],
    visibility = ["//score/test/integration:__subpackages__"],
)

py_library(
    name = "logging_plugin_lib",
    srcs = ["logging_plugin.py"],
    imports = ["."],
    deps = ["@score_itf//score/itf/plugins:docker"],
)

py_itf_plugin(
    name = "logging_plugin",
    enabled_plugins = ["logging_plugin"],
    py_library = ":logging_plugin_lib",
    visibility = ["//score/test/integration:__subpackages__"],
)
