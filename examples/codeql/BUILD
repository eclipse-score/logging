# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
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

# Compilation target so CodeQL can trace the file and report findings.
# `copts = ["-w"]` disables compiler warnings — the file deliberately violates
# rules. Without -w, `warnings_as_errors` from the project .bazelrc would fail
# the build before CodeQL can analyze it.
# Do NOT use this binary in production or test pipelines.
cc_binary(
    name = "misra_cert_violations",
    srcs = ["misra_cert_violations.cpp"],
    copts = ["-w"],
)
