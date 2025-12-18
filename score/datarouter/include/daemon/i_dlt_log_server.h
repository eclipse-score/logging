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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_DLT_LOG_SERVER_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_DLT_LOG_SERVER_H

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

using loglevel_t = mw::log::LogLevel;
enum class ThresholdCmd : std::uint8_t
{
    UseDefault = 0xFF
};
using threshold_t = std::variant<loglevel_t, ThresholdCmd>;

enum class AssignmentAction : std::uint8_t
{
    Remove = 0,
    Add = config::DLT_ASSIGN_ADD
};

// This class is responsible for providing an interface for diagnostic job handling
class IDltLogServer
{
  public:
    virtual const std::string ReadLogChannelNames() = 0;
    virtual const std::string ResetToDefault() = 0;
    virtual const std::string StoreDltConfig() = 0;
    virtual const std::string SetTraceState() = 0;
    virtual const std::string SetDefaultTraceState() = 0;
    virtual const std::string SetLogChannelThreshold(score::platform::dltid_t channel, loglevel_t threshold) = 0;
    virtual const std::string SetLogLevel(score::platform::dltid_t appId,
                                          score::platform::dltid_t ctxId,
                                          threshold_t threshold) = 0;
    virtual const std::string SetMessagingFilteringState(bool enabled) = 0;
    virtual const std::string SetDefaultLogLevel(loglevel_t level) = 0;
    virtual const std::string SetLogChannelAssignment(score::platform::dltid_t appId,
                                                      score::platform::dltid_t ctxId,
                                                      score::platform::dltid_t channel,
                                                      AssignmentAction assignment_flag) = 0;
    virtual const std::string SetDltOutputEnable(bool enable) = 0;

    virtual ~IDltLogServer() = default;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_DLT_LOG_SERVER_H
