load("@//third_party:codecraft_remotes.bzl", "CC_UBUNTU_ARCHIVE")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@swh_bazel_rules//artifactory:artifactory_deb.bzl", "artifactory_deb_group")

def libatomic():
    maybe(
        artifactory_deb_group,
        name = "libatomic",
        build_file = "//third_party/libatomic:libatomic.BUILD",
        repo = CC_UBUNTU_ARCHIVE,
        package_group = {
            "pool/main/g/gcc-10//libatomic1_10.5.0-1ubuntu1~20.04_amd64.deb": "29b19adba1cb79b0cbf905ee21cc68d3a88eadcd56c39d1a0b29c5a523b60557",
        },
    )
