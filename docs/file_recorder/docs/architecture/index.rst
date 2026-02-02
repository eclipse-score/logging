..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
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

.. _component_architecture_file_recorder:

file_recorder Component Architecture
====================================

.. document:: file_recorder Architecture
   :id: doc__file_recorder_architecture
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__component_arch
   :tags: template

.. attention::
    The above directive must be updated according to your needs.

    - Modify ``Your Component Name`` to be your Component Name
    - Modify ``id`` to be your Component Name in upper snake case preceded by ``doc__`` and followed by ``_architecture``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs

Overview
--------
<Brief summary>

Requirements Linked to Component Architecture
---------------------------------------------

.. code-block:: none

   .. needtable:: Overview of Component Requirements
      :style: table
      :columns: title;id
      :filter: search("comp_arch_sta__archdes$", "fulfils_back")
      :colwidths: 70,30

Description
-----------

<General Description>

<Design Constraints>

Rationale Behind Architecture Decomposition
*******************************************

Mandatory: A motivation for the decomposition or reason for not further splitting it into internal components.

.. note:: Common decisions across components / cross cutting concepts is at the higher level.

Static Architecture
-------------------

The components are designed to cover the expectations from the feature architecture
(i.e. if already exists a definition it should be taken over and enriched).

.. comp:: file_recorder
   :id: comp__file_recorder
   :security: YES
   :safety: ASIL_B
   :status: invalid
   :implements: logic_arc_int__log_cpp__recorder   
   :uses: logic_arc_int__baselibs__filesystem

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}   
..
   TEMPLATE: Uncomment and fill out the following comp_arc_sta directive:

   .. comp_arc_sta:: Component Name (Static View)
      :id: comp_arc_sta__component_name__static_view
      :security: YES
      :safety: ASIL_B
      :status: invalid
      :includes: comp_arc_sta__component_name__2

      .. needarch::
         :scale: 50
         :align: center

         {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

..
   TEMPLATE: Uncomment and customize the comp_arc_dyn directive:

   .. comp_arc_dyn:: Dynamic View
      :id: comp_arc_dyn__component_name__dynamic_view
      :security: YES
      :safety: ASIL_B
      :status: invalid

      Put here a sequence diagram


Interfaces
----------
..
   .. real_arc_int:: <Title>
      :id: real_arc_int__<component>__<Title>
      :security: <YES|NO>
      :safety: <QM|ASIL_B>
      :fulfils: <link to component requirement id>
      :language: cpp

Internal Components
-------------------

..
   TEMPLATE: Uncomment and customize the comp_arc_sta directive:

   .. comp_arc_sta:: Component Name Static View
      :id: comp_arc_sta__component_name__2
      :status: invalid
      :safety: ASIL_B
      :security: YES

      No architecture but detailed design

.. note::
   Architecture can be split into multiple files. At component level the public interfaces to be used by the user and tester to be shown.

.. attention::
    The above directives must be updated according to your component architecture.

    - Replace the example content by the real content
    - Set the status to valid and start the review/merge process
