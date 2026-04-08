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

"""Clang-tidy linter configuration for score_logging."""

load("@score_cpp_policies//clang_tidy:defs.bzl", "make_clang_tidy_aspect", "make_clang_tidy_test")

clang_tidy_aspect = make_clang_tidy_aspect(
    binary = Label("@llvm_toolchain//:clang-tidy"),
    configs = [Label("//:.clang-tidy")],
)

clang_tidy_test = make_clang_tidy_test(aspect = clang_tidy_aspect)
