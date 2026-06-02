# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")
load("@score_itf//:defs.bzl", "py_itf_test")
load("@score_rules_imagefs//rules/qnx:ifs.bzl", "qnx_ifs")

_ENV = "//quality/integration_testing/environments/qnx8_qemu"

def _extend_list_in_kwargs(kwargs, key, values):
    kwargs[key] = kwargs.get(key, []) + values
    return kwargs

def py_logging_itf_test(name, srcs, filesystem, **kwargs):
    """Integration test macro for score_logging (Docker and QNX).

    Args:
        name: Test target name.
        srcs: Python test source files.
        filesystem: pkg_tar target with test-specific binaries and configs.
        **kwargs: Forwarded to py_itf_test.
    """
    image_name = "_image_{}".format(name)
    image_loader = "_image_{}_loader".format(name)
    repo_tag = "{}:latest".format(name)

    oci_image(
        name = image_name,
        base = "@ubuntu_24_04",
        tars = [
            "//score/test/component:dlt_pkg",
            "//score/test/component:datarouter_pkg",
            filesystem,
        ],
        target_compatible_with = ["@platforms//os:linux"],
    )

    oci_load(
        name = image_loader,
        image = image_name,
        repo_tags = [repo_tag],
        target_compatible_with = ["@platforms//os:linux"],
    )

    qnx_image = "_qnx_ifs_{}".format(name)
    qnx_ifs(
        name = qnx_image,
        srcs = [
            filesystem,
            "//score/datarouter",
            "//score/test/component/datarouter:datarouter_test_config_files",
            "{}:qnx8_qemu_env".format(_ENV),
        ],
        build_file = "{}:init_build".format(_ENV),
        ext_repo_maping = {
            "FILESYSTEM": "$(location {})".format(filesystem),
            "DATAROUTER": "$(location //score/datarouter)",
        },
        target_compatible_with = select({
            "@platforms//cpu:x86_64": ["@platforms//cpu:x86_64"],
            "@platforms//cpu:arm64": ["@platforms//cpu:arm64"],
        }) + ["@platforms//os:qnx"],
    )

    _extend_list_in_kwargs(kwargs, "data", select({
        "@platforms//os:qnx": [
            qnx_image,
            "{}:qemu_bridge_config.json".format(_ENV),
            "{}:dlt_config_qnx.json".format(_ENV),
        ],
        "//conditions:default": [image_loader],
    }))

    _extend_list_in_kwargs(kwargs, "args", select({
        "@platforms//os:qnx": [
            "--log-cli-level=DEBUG",
            "--qemu-image=$(location {})".format(qnx_image),
            "--qemu-config=$(location {}:qemu_bridge_config.json)".format(_ENV),
            "--dlt-config=$(location {}:dlt_config_qnx.json)".format(_ENV),
        ],
        "//conditions:default": [
            "--log-cli-level=DEBUG",
            "--docker-image-bootstrap=$(location {})".format(image_loader),
            "--docker-image={}".format(repo_tag),
        ],
    }))

    if "tags" not in kwargs:
        kwargs["tags"] = []
    if "integration" not in kwargs["tags"]:
        kwargs["tags"] = kwargs["tags"] + ["integration"]

    if "size" not in kwargs:
        kwargs["size"] = "enormous"

    if "timeout" not in kwargs:
        kwargs["timeout"] = "short"

    py_itf_test(
        name = name,
        srcs = srcs,
        plugins = [
            "@score_itf//score/itf/plugins:dlt_plugin",
            "//score/test/component:logging_plugin",
        ] + select({
            "@platforms//os:qnx": ["@score_itf//score/itf/plugins:qemu_plugin"],
            "//conditions:default": ["@score_itf//score/itf/plugins:docker_plugin"],
        }),
        env = {"DOCKER_HOST": ""},
        **kwargs
    )
