Logging Module View
###################


.. mod:: Logging
   :id: mod__logging_repo
   :status: valid
   :safety: ASIL_B
   :security: YES
   :includes: comp__data_router, comp__middleware

   The logging module provides a standardized logging framework for C++ and Rust projects using Bazel. It includes components for log routing and middleware processing, allowing for flexible log management and integration with various logging backends. The module is designed to be extensible and configurable to meet the needs of different applications and environments.

.. mod_view_sta:: Logging module view
   :id: mod_view_sta__logging__logging_view
   :includes: comp__data_router, comp__middleware
   :belongs_to: mod__logging

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_module(need(), needs) }}



Logging Feature Architecture
############################

.. feat_arc_sta:: Feature Architecture Logging
   :id: feat_arc_sta__logging__static_view
   :security: YES
   :safety: ASIL_B
   :status: valid
   :includes: logic_arc_int__logging__logging
   :fulfils: feat_req__logging__log_sources_user_app
   :belongs_to: feat__logging

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}
