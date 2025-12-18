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

#ifndef DATAROUTER_ETC_H_
#define DATAROUTER_ETC_H_

#include <score/string_view.hpp>

namespace score
{
namespace logging
{
namespace config
{
static const std::string socket_address{"datarouter_socket"};
static constexpr const score::cpp::string_view dlt_config_client_name{"_dlt_config"};
}  // namespace config
}  // namespace logging
}  // namespace score

#endif  // DATAROUTER_ETC_H_
