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

load("@score_qnx_unit_tests//:defs.bzl", "cc_test_qnx")

def py_unittest_qnx_test(
        name,
        test_cases = [],
        test_suites = [],
        data_files = [],  # unused: cc_test_qnx has no data param, data must live on the wrapped cc_test
        excluded_tests_filter = None,
        visibility = None):
    """Wraps cc_test targets with cc_test_qnx and aggregates them into a test_suite.

    Args:
      name: Name of the resulting test_suite.
      test_cases: cc_test targets to execute on QNX via cc_test_qnx.
      test_suites: Already QNX-wrapped test_suite/test targets to include as-is.
      data_files: Unused, see note above.
      excluded_tests_filter: Forwarded to each generated cc_test_qnx (gtest filters).
      visibility: Forwarded to the resulting test_suite.
    """
    qnx_tests = []
    for test_case in test_cases:
        qnx_name = "{}_{}".format(name, test_case.lstrip(":").replace("/", "_").replace(":", "_"))
        cc_test_qnx(
            name = qnx_name,
            cc_test = test_case,
            excluded_tests_filter = excluded_tests_filter,
        )
        qnx_tests.append(":" + qnx_name)

    native.test_suite(
        name = name,
        tests = qnx_tests + test_suites,
        visibility = visibility,
    )
