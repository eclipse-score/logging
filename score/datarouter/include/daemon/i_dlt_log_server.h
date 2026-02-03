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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_I_DLT_LOG_SERVER_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_I_DLT_LOG_SERVER_H

#include "score/mw/log/log_level.h"
#include "score/datarouter/include/daemon/configurator_commands.h"
#include "score/datarouter/include/dlt/dltid.h"
#include <variant>

namespace score
{
namespace logging
{
namespace dltserver
{

using LoglevelT = mw::log::LogLevel;
enum class ThresholdCmd : std::uint8_t
{
    kUseDefault = 0xFF
};
using ThresholdT = std::variant<LoglevelT, ThresholdCmd>;

enum class AssignmentAction : std::uint8_t
{
    kRemove = 0,
    kAdd = config::kDltAssignAdd
};

// This class is responsible for providing an interface for diagnostic job handling
class IDltLogServer
{
  public:
    virtual std::string ReadLogChannelNames() = 0;
    virtual std::string ResetToDefault() = 0;
    virtual std::string StoreDltConfig() = 0;
    virtual std::string SetTraceState() = 0;
    virtual std::string SetDefaultTraceState() = 0;
    virtual std::string SetLogChannelThreshold(score::platform::DltidT channel, LoglevelT threshold) = 0;
    virtual std::string SetLogLevel(score::platform::DltidT app_id,
                                    score::platform::DltidT ctx_id,
                                    ThresholdT threshold) = 0;
    virtual std::string SetMessagingFilteringState(bool enabled) = 0;
    virtual std::string SetDefaultLogLevel(LoglevelT level) = 0;
    virtual std::string SetLogChannelAssignment(score::platform::DltidT app_id,
                                                score::platform::DltidT ctx_id,
                                                score::platform::DltidT channel,
                                                AssignmentAction assignment_flag) = 0;
    virtual std::string SetDltOutputEnable(bool enable) = 0;

    virtual ~IDltLogServer() = default;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_I_DLT_LOG_SERVER_H
