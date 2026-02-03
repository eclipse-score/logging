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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_DATA_ROUTER_CFG_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_DATA_ROUTER_CFG_H

#include <score/string_view.hpp>

namespace score
{
namespace logging
{
namespace config
{
static const std::string kSocketAddress{"datarouter_socket"};
static constexpr const score::cpp::string_view kDltConfigClientName{"_dlt_config"};
}  // namespace config
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_DATA_ROUTER_CFG_H
