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

Requirements
############

.. document:: Logging Requirements
   :id: doc__logging_requirements
   :status: valid
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp

Component Requirements
----------------------

.. comp_req:: Key Naming
   :id: comp_req__logging__something
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :satisfies: feat_req__logging__timestamping_original
   :status: valid
   :belongs_to: comp__logging

   The component shall accept keys that consist solely of alphanumeric characters, underscores, or dashes.
