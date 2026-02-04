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

.. _feature_architecture_logging:

Logging Feature Architecture
============================

.. document:: logging Architecture
   :id: doc__logging_architecture
   :status: draft
   :safety: ASIL_B
   :security: NO
   :realizes: wp__feature_arch
   :tags: template

.. attention::
    The above directive must be updated according to your Feature.

    - Modify ``Your Feature Name`` to be your Feature Name
    - Modify ``id`` to be your Feature Name in upper snake case preceded by ``doc__`` and followed by ``_architecture``
    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs

Overview
--------
Brief summary

Description
-----------

General Description

.. Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.

Design Constraints

Requirements
------------

.. code-block:: none

   .. needtable:: Overview of Feature Requirements
      :style: table
      :columns: title;id
      :filter: search("feat_arch_sta__archdes$", "fulfils_back")
      :colwidths: 70,30


Rationale Behind Architecture Decomposition
*******************************************
mandatory: a motivation for the decomposition

.. note:: Common decisions across features / cross cutting concepts is at the high level.

Static Architecture
-------------------
..
   .. feat_arc_sta:: Static View
      :id: feat_arc_sta__logging__static_view
      :security: YES
      :safety: ASIL_B
      :status: valid
      :fulfils: feat_req__logging__timestamping_original
      :includes: feat__logging
      .. needarch::
         :scale: 50
         :align: center

         {{ draw_feature(need(), needs) }}


.. feat:: Logging
   :id: feat__logging
   :security: YES
   :safety: ASIL_B
   :status: valid
   :includes: logic_arc_int__log_cpp__logging, logic_arc_int__log_rust__logging_rust
   :consists_of: comp__log_rust, comp__log_cpp, comp__log_bridge, comp__datarouter, comp__recorders
   
   .. needarch::
      :scale: 50
      :align: center

      {{ draw_feature(need(), needs) }}

Dynamic Architecture
--------------------

.. .. feat_arc_dyn:: Dynamic View
..    :id: feat_arc_dyn__logging__dynamic_view
..    :security: YES
..    :safety: ASIL_B
..    :status: invalid
..    :fulfils: feat_req__feature_name__some_title

   put here a sequence diagram

Logical Interfaces
------------------




.. logic_arc_int:: buffer
   :id: logic_arc_int__logging__buffer
   :security: YES
   :safety: ASIL_B
   :status: valid





.. logic_arc_int_op:: Log
   :id: logic_arc_int_op__logging__log
   :security: YES
   :safety: ASIL_B
   :status: valid
   :included_by: logic_arc_int__log_cpp__logging


Architecture draft from CFT discussion
--------------------------------------

.. figure:: ../../_assets/feat_arc_sta__log__logging.drawio.svg
   :align: center
   :name: fr_logging_arc

   Architecture draft from CFT discussion
