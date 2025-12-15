cc_library(
    name = "libatomic",
    linkopts = select({
        "@platforms//os:linux": ["-latomic"],  # Link actual system libatomic
        "@platforms//os:qnx": ["-latomic"],  # QNX also needs libatomic
        "//conditions:default": [],  # Other platforms don't need it
    }),
    visibility = ["//visibility:public"],
)
