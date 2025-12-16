/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#ifndef SCORE_PAS_LOGGING_NETWORK_VLAN_H
#define SCORE_PAS_LOGGING_NETWORK_VLAN_H

#if defined(USE_LOCAL_VLAN)
#include "score/datarouter/network/vlan_local.h"
#else
#include "score/network/vlan.h"
#endif

#endif  // SCORE_PAS_LOGGING_NETWORK_VLAN_H
