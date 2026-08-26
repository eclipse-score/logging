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

def py_unittest_qnx_test(
        name,
        test_cases = [],
        test_suites = [],
        data_files = [],  # unused: cc_test_qnx has no data param, data must live on the wrapped cc_test
        excluded_tests_filter = None,
        visibility = None):
    # This is intentionally left blank.
    # The cc_test_qnx from @score_qnx_unit_tests can be used here
    # See: eclipse-score/logging/issues/267
    pass
