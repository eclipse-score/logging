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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_UTILITY_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_UTILITY_H

#include "score/mw/log/log_level.h"
#include <string>
#include <unordered_map>

namespace logging_daemon
{
namespace logchannel_utility
{

score::mw::log::LogLevel ToLogLevel(const std::string& logLevel);

std::string ToString(const score::mw::log::LogLevel level);

inline std::string GetStringFromLogLevel(score::mw::log::LogLevel level)
{
    return ToString(level);
}

}  // namespace logchannel_utility
}  // namespace logging_daemon

namespace logchannel_operations = logging_daemon::logchannel_utility;

#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_UTILITY_H
