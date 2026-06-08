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

Datarouter Component Requirements
##################################

.. document:: Datarouter Component Requirements
   :id: doc__data_router_requirements
   :status: valid
   :safety: QM
   :security: NO
   :realizes: wp__requirements_comp

Datarouter Services
-------------------

.. comp_req:: Datarouter DLT Server
   :id: comp_req__data_router__dlt_server
   :reqtype: Functional
   :security: NO
   :safety: QM
   :satisfies: feat_req__logging__compat_dlt, feat_req__logging__log_sinks_network
   :status: valid
   :belongs_to: comp__data_router

   Datarouter shall implement DLT server component. Datarouter provides the functionality to
   send messages using DLT protocol over UDP multicast transport.

.. comp_req:: DLT Multiple Channel Support
   :id: comp_req__data_router__dlt_multiple_channels
   :reqtype: Functional
   :security: NO
   :safety: QM
   :satisfies: feat_req__logging__compat_dlt, feat_req__logging__config_log_filter, feat_req__logging__config_entity_id
   :status: valid
   :belongs_to: comp__data_router

   DLT server shall support multiple log channels as specified in AUTOSAR Diagnostic, Log and
   Trace Protocol Specification. The following parameters shall be configurable statically via
   a deployment-specific configuration file
   (``<data_router binary location>/./etc/log-channels.json``): Number and names of channels,
   Network configuration for channel output, Log level threshold per channel, Log channel
   assignments per application/context.

.. comp_req:: DLT Message Filtering
   :id: comp_req__data_router__dlt_message_filtering
   :reqtype: Functional
   :security: NO
   :safety: QM
   :satisfies: feat_req__logging__filtering_log_levels, feat_req__logging__filtering_entity_id, feat_req__logging__config_log_filter
   :status: valid
   :belongs_to: comp__data_router

   DLT server shall support filtering of messages within each channel. The filter shall be
   parametrized with application ID, context ID, log level threshold. Initial filter set shall
   be part of channel configuration file, located in the following path:
   ``<data_router binary location>/../etc/log-channels.json``.

.. comp_req:: DLT Verbose Messages
   :id: comp_req__data_router__dlt_verbose_messages
   :reqtype: Functional
   :security: NO
   :safety: QM
   :satisfies: feat_req__logging__compat_dlt
   :status: valid
   :belongs_to: comp__data_router

   DLT server shall provide support for verbose DLT messages. Messages sent from mw::log
   library shall be sent via DLT using verbose mode.

DLT Quotas
----------

.. comp_req:: DLT Bandwidth Quota Configuration
   :id: comp_req__data_router__dlt_bw_quota_config
   :reqtype: Functional
   :security: NO
   :safety: QM
   :satisfies: feat_req__logging__config_buffer_size, feat_req__logging__resource_comm_channel
   :status: valid
   :belongs_to: comp__data_router

   The component shall have the DLT bandwidth quota configuration in the file located at
   ``./etc/log-channels.json`` relative to the application specific location ``/opt/``.

.. comp_req:: DLT Quota Bandwidth Reduction
   :id: comp_req__data_router__dlt_quota_bw_reduction
   :reqtype: Functional
   :security: NO
   :safety: QM
   :satisfies: feat_req__logging__message_loss_handling, feat_req__logging__qos_message_handling
   :status: valid
   :belongs_to: comp__data_router

   It shall be possible to configure the action to drop DLT messages when a quota is exceeded.

   Note: The configuration shall be provided as an attribute as part of the DLT quota
   configuration file (:need:`comp_req__data_router__dlt_bw_quota_config`).
