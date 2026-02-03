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

#ifndef SCORE_DATAROUTER_INCLUDE_ROUTER_DATA_ROUTER_TYPES_H
#define SCORE_DATAROUTER_INCLUDE_ROUTER_DATA_ROUTER_TYPES_H

#include <cstdint>
#include <string>

namespace score
{
namespace platform
{

using BufsizeT = std::uint32_t;

struct DataFilter
{
    std::string filter_type;
    std::string filter_data;
};

}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_ROUTER_DATA_ROUTER_TYPES_H
