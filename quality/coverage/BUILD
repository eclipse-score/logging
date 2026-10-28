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

load("@score_coverage//:defs.bzl", "score_coverage_reporter", "score_coverage_scope")

# The production components of this module — the same scope the previous gcov
# configuration expressed as an instrumentation filter (//score/datarouter,
# //score/mw/log minus tests). The scope aspect walks the roots' transitive
# in-workspace deps; test doubles (//score/mw/log/test/...), examples
# (custom_recorder_example) and external deps are not reachable from these
# roots and stay out of the report. Untested in-scope files appear at exact
# 0% via the --empty-profile baseline.
#
# KNOWN GAP: the cc_binary entry-point sources of //score/datarouter:datarouter
# (main) are not collected — cc_binary provides no CcInfo for the scope aspect;
# its library deps are. Tracked as a scope-aspect enhancement in score_coverage.
score_coverage_scope(
    name = "coverage_scope",
    testonly = True,
    visibility = ["//visibility:private"],
    deps = [
        # Logging backend (pulls in all the backends it wires up)
        "//score/mw/log",
        # Backend not reachable through the facade. (//score/mw/log/backend:slog
        # is QNX-only and cannot be measured on the Linux host; on-target
        # coverage is tracked in eclipse-score/tooling#427.)
        "//score/mw/log/backend:custom",
        # Linux syslog backend, not reachable through the facade.
        "//score/mw/log/backend:syslog",
        # Rust bridge crate + its C++ init library
        "//score/mw/log/rust/score_log_bridge",
        "//score/mw/log/rust/score_log_bridge_cpp_init",
        # Datarouter daemon (walks its application libraries)
        "//score/datarouter",
    ],
)

score_coverage_reporter(
    name = "reporter_wrapper",
    testonly = True,
    coverage_scope = ":coverage_scope",
    llvm_cov = "@llvm_toolchain_coverage//:llvm-cov",
    llvm_cxxfilt = "@llvm_toolchain_coverage_llvm//:bin/llvm-cxxfilt",
    llvm_profdata = "@llvm_toolchain_coverage//:llvm-profdata",
)

# QNX on-target coverage (C++ only; the gcov backend cannot measure Rust).
# Follows the run's platform so the QNX-only slog backend resolves.
score_coverage_scope(
    name = "coverage_scope_qnx",
    testonly = True,
    platform = "@score_bazel_platforms//:x86_64-qnx-sdp_8.0.0-posix",
    # Manual: a wildcard build on a Linux host has no QNX toolchains registered.
    tags = ["manual"],
    visibility = ["//visibility:private"],
    deps = [
        "//score/datarouter",
        "//score/mw/log",
        "//score/mw/log/backend:custom",
        "//score/mw/log/backend:slog",
    ],
)

# QCC is GCC-based, so the counters are gcov's. Selected by --config=qnx_cov.
score_coverage_reporter(
    name = "gcov_reporter_wrapper",
    testonly = True,
    backend = "gcov",
    coverage_scope = ":coverage_scope_qnx",
    gcov = "@score_qcc_x86_64_toolchain_pkg//:gcov",
    # Manual: needs the QNX SDP package, which requires QNX credentials.
    tags = ["manual"],
)
