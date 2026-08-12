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

- ``score/mw/log/``: ``mw::log`` recorders and concrete backends and Rust bridge
- ``score/datarouter/``: DLT daemon and supporting libraries
- ``test/``: Component and Integration tests
- ``docs/``: Documentation using ``docs-as-code``
- ``.github/workflows/``: CI/CD pipelines

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

   bazel build //score/...

To run the supported test suites:

.. code-block:: bash

   # Unit tests on the Linux host
   bazel test --config=x86_64-linux --test_tag_filters=unit //score/...

   # Component tests in Docker
   bazel test --config=x86_64-linux --test_tag_filters=integration //score/test/component/...

   # Integration tests on QNX/QEMU
   bazel test --config=x86_64-qnx --test_tag_filters=integration //score/test/integration/...

Configuration
-------------

See the `mw::log configuration documentation`_.

.. _mw::log configuration documentation: https://github.com/eclipse-score/baselibs/blob/main/score/mw/log/README.md#configuration

Stats
-----

.. toctree::
   :titlesonly:
   :maxdepth: 1

   logging/stats.rst
