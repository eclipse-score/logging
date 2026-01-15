#!/usr/bin/env bash

set -e

# Manual targets are not take into account, must be set explicitly
bazel run @rules_rust//tools/rust_analyzer:gen_rust_project -- "@//examples/..." "@//score/..." "@//tests/..."
