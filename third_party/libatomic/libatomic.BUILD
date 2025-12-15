package(default_visibility = ["//visibility:public"])

libatomic_libs = [
    "usr/lib/x86_64-linux-gnu/libatomic.so.1",
    "usr/lib/x86_64-linux-gnu/libatomic.so.1.2.0",
]

cc_library(
    name = "libatomic",
    srcs = libatomic_libs,
    visibility = [
        "//visibility:public",
    ],
)

# A dedicated filegroup that exposes only the shared libraries (without headers).
# This is used to provide .so files to packaging tools (e.g., for .deb creation),
# which do not require headers or full C++ dependency metadata.
# It simplifies packaging logic and avoids unintentionally pulling in development files.
filegroup(
    name = "libatomic_solibs",
    srcs = libatomic_libs,
)
