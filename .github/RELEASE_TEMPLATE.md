<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

Module Name: logging
Release Tag: $RELEASE_TAG
Origin Release Tag: $PREVIOUS_RELEASE_TAG
Release Commit Hash: $COMMIT_SHA
Release Date: $RELEASE_DATE

## Overview

The logging module provides components (`mw::log`, `datarouter`, `score_log_bridge`) for safety-critical logging (Diagnostic Log and Trace) in embedded automotive systems.

## Disclaimer

This release is not intended for production use, as it does not include a safety argumentation or a completed safety assessment.
The work products compiled in the safety package are created with care according to the S-CORE process (https://eclipse-score.github.io/process_description/main/index.html). However, as a non-profit, open-source organization, the project cannot assume any liability for its content.

For details on the features, see https://eclipse-score.github.io/logging/main/features/logging/index.html

## Notable Changes

## Compatibility

The following platforms are supported using the bazel_cpp_toolchains (https://github.com/eclipse-score/bazel_cpp_toolchains):
- `x86_64-unknown-linux-gnu`
- `aarch64-unknown-linux-gnu`
- `x86_64-unknown-nto-qnx800`
- `aarch64-unknown-nto-qnx800`

## Performed Verification

- Unit test execution with LLVM source-based coverage (C++ and Rust) on Linux x86_64
- Unit test execution with gcov coverage (C++) on QNX x86_64 in QEMU

The Linux and QNX coverage reports are attached as separate archives, because the two coverage backends are not comparable line by line.

Report: $ACTION_RUN_URL

## Known Issues

<!-- Remove this section entirely if there are no known issues to report. -->

## Upgrade Instructions

Backward compatibility with the previous release is not guaranteed.

## Contact Information

For any questions or support, please raise an issue/discussion in the logging repository (https://github.com/eclipse-score/logging).
