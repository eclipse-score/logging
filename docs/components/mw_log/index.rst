..
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


Logging Backend Component
#########################

.. document:: Logging Backend
   :id: doc__logging_backend
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__cmpt_request

Abstract
========

Supports logging on several backends (file,slog,remote,custom).

The ``mw::log`` implementation spans the ``score_baselibs`` (frontend) and ``score_logging`` (backend)
repositories; see the :doc:`feature architecture </features/logging/architecture/index>` for the repository boundary
and cross-repository contract.


Specification
=============

see :need:`doc__logging_backend_requirements`


How to Teach This
=================

[How to teach users, new and experienced, how to apply the CR to their work.]

.. note::
   For a CR that adds new functionality or changes behaviour, it is helpful to include a section on how to teach users, new and experienced, how to apply the CR to their work.

Footnotes
=========

[A collection of footnotes cited in the CR, and a place to list non-inline hyperlink targets.]


Further Documentation of the component can be found in the following sections:

Component Detail Information
============================

.. toctree::
   :maxdepth: 1

   architecture/index
   detailed_design/index
   requirements/index
   safety_analysis/dfa
   safety_analysis/fmea
   safety_analysis/aou_requirements
