"""Third-party dependencies configuration for eclipse-score-logging.

This module manages external dependencies for the eclipse-score-logging project.
Currently, only essential dependencies required for the minimal logging toolchain
are loaded and initialized.

Active dependencies:
    - libatomic: Atomic operations library support

Usage:
    Load this module in your WORKSPACE file and call third_party_deps() to
    initialize all third-party dependencies.
"""

load("@//third_party/libatomic:libatomic.bzl", "libatomic")

def third_party_deps():
    libatomic()
