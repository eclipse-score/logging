load("@rules_python//python:defs.bzl", "py_binary")

package(default_testonly = True)

py_binary(
    name = "dlt_stats",
    srcs = [
        "dlt_stats.py",
    ],
    data = [
        "//platform/aas/tools/python_dlt:dlt-artifacts",
        "//third_party/dltdump",
    ],
    deps = [
        "//platform/aas/tools/python_dlt",
    ],
)
