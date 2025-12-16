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

#include "score/datarouter/include/daemon/utility.h"
#include <iostream>

namespace logging_daemon
{
namespace logchannel_utility
{

const std::unordered_map<std::string, score::mw::log::LogLevel> loglevel_map = {
    {"kOff", score::mw::log::LogLevel::kOff},
    {"kFatal", score::mw::log::LogLevel::kFatal},
    {"kError", score::mw::log::LogLevel::kError},
    {"kWarn", score::mw::log::LogLevel::kWarn},
    {"kInfo", score::mw::log::LogLevel::kInfo},
    {"kDebug", score::mw::log::LogLevel::kDebug},
    {"kVerbose", score::mw::log::LogLevel::kVerbose}};

score::mw::log::LogLevel ToLogLevel(const std::string& logLevel)
{
    score::mw::log::LogLevel mappedLevel = score::mw::log::LogLevel::kOff;
    auto it = loglevel_map.find(logLevel);
    if (it != loglevel_map.end())
    {
        mappedLevel = it->second;
    }
    else
    {
        std::cerr << "Invalid Log Level String!!!:" << logLevel << std::endl;
    }
    return mappedLevel;
}
std::string ToString(const score::mw::log::LogLevel level)
{
    std::string result;

    switch (level)
    {
        case score::mw::log::LogLevel::kOff:
            result = "kOff";
            break;
        case score::mw::log::LogLevel::kFatal:
            result = "kFatal";
            break;
        case score::mw::log::LogLevel::kError:
            result = "kError";
            break;
        case score::mw::log::LogLevel::kWarn:
            result = "kWarn";
            break;
        case score::mw::log::LogLevel::kInfo:
            result = "kInfo";
            break;
        case score::mw::log::LogLevel::kVerbose:
            result = "kVerbose";
            break;
        case score::mw::log::LogLevel::kDebug:
            result = "kDebug";
            break;
        default:
            result = "undefined";
            break;
    }

    return result;
}

}  // namespace logchannel_utility
}  // namespace logging_daemon
