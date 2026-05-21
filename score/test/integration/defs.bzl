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

def _extend_list_in_kwargs(kwargs, key, values):
    kwargs[key] = kwargs.get(key, []) + values
    return kwargs

def py_logging_itf_test(name, srcs, filesystem, **kwargs):
    """Integration test macro for score_logging using Docker + DLT plugins.

    Builds a per-test Docker image from the given filesystem tar layers
    (which must include the test app binaries/configs) combined with the
    shared base layers (DLT daemon, datarouter).

    Args:
        name: Test target name.
        srcs: Python test source files.
        filesystem: A pkg_tar target containing the test-specific binaries
                    and configuration files.
        **kwargs: Forwarded to py_itf_test (e.g. deps, data, tags, timeout).
    """
    image_name = "_image_{}".format(name)
    image_loader = "_image_{}_loader".format(name)
    repo_tag = "{}:latest".format(name)

    oci_image(
        name = image_name,
        base = "@ubuntu_24_04",
        tars = [
            "//score/test/integration:dlt_pkg",
            "//score/test/integration:datarouter_pkg",
            filesystem,
        ],
    )

    oci_load(
        name = image_loader,
        image = image_name,
        repo_tags = [repo_tag],
    )

    _extend_list_in_kwargs(kwargs, "data", [image_loader])
    _extend_list_in_kwargs(kwargs, "args", [
        "--docker-image-bootstrap=$(location {})".format(image_loader),
        "--docker-image={}".format(repo_tag),
    ])

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
            "@score_itf//score/itf/plugins:docker_plugin",
            "@score_itf//score/itf/plugins:dlt_plugin",
            "//score/test/integration:logging_plugin",
        ],
        env = {"DOCKER_HOST": ""},
        **kwargs
    )
