..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
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

Logging
=====================

This module provides components for safety-critical logging (incl. DLT) in embedded automotive systems.

.. contents:: Table of Contents
   :depth: 2
   :local:

Overview
--------

``score_logging`` delivers three distinct logging components:

- ``mw::log`` is the logging middleware library.
   Applications use the frontend API owned by `score_baselibs <https://github.com/eclipse-score/baselibs/blob/main/score/mw/log/README.md>`_,
   while `score_logging <https://github.com/eclipse-score/logging/tree/main/score/mw/log>`_ provides the concrete
   file, remote/DLT, and system recorder backend implementations.
- ``datarouter`` is the DLT daemon executable.
   It reads records from source shared-memory ring buffers, manages source
   sessions, routes messages to configured channels via UDP multicast, and reports source statistics and message drops.
- ``score_log_bridge`` is a Rust library that implements the ``score_log`` facade
   using the C++ ``mw::log`` recorder
   through a layout-checked FFI adapter.

Repository Layout
-----------------

The logging module includes the following top-level structure:

- ``docs/``: Documentation using ``docs-as-code``
- ``examples/``: Dash-tool license-check
- ``quality/``: integration-test (QEMU) environment configs and sanitizer suppression files (ASan/LSan/TSan/UBSan).
- ``score/mw/log/``: ``mw::log`` recorders and concrete backends and Rust bridge
- ``score/datarouter/``: DLT daemon and supporting libraries
- ``score/test/``: Component and Integration tests
- ``scripts/``: developer-tooling helpers
- ``third_party/``: bazel wiring for external tooling deps
- ``tools/``: developer-tooling
- ``.github/workflows/``: CI/CD pipelines

Architecture
------------

.. toctree::
   :titlesonly:
   :maxdepth: 1

   features/architecture/index

Components
----------

.. toctree::
   :titlesonly:
   :maxdepth: 1
   :glob:

   components/mw_log/index.rst
   components/datarouter/index.rst

Quick Start
-----------

To build the module:

.. code-block:: bash

   bazel build --config=x86_64-linux //score/...

To run the supported test suites:

.. code-block:: bash

   # Unit tests on the Linux host
   bazel test --config=x86_64-linux --test_tag_filters=unit //score/...

   # Component tests in Docker
   bazel test --config=x86_64-linux --test_tag_filters=integration //score/test/component/...

   # Integration tests on QNX/QEMU
   bazel test --config=x86_64-qnx --test_tag_filters=integration //score/test/integration/...

Quality
-------

Existing tooling and the KPIs each one gates in CI (see ``.github/workflows/``):

- **Static analysis** — ``clang-tidy`` on every C++ target; ``clippy``/``rustfmt`` (``rustfmt.toml``) for Rust.
- **Sanitizers** — ASan/UBSan/LSan/TSan runs per PR, suppressions are placed in ``quality/sanitizer/``.
- **Code coverage** — C++ line/branch coverage report, excluding integration-tagged tests.
- **Test suites** — Google Test/Rust unit tests (Linux), component (Docker), and integration (QNX/QEMU) tests run separately.
- **License compliance** — Dash license scan of C++/Rust dependencies.
- **Copyright & format** — header and formatting checks on every change.

Stats
-----

.. toctree::
   :titlesonly:
   :maxdepth: 1

   verification_report/stats.rst

Guides
---------

.. toctree::
   :titlesonly:
   :maxdepth: 1

   module/manuals/index
