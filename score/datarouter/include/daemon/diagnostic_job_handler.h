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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_DIAGNOSTIC_JOB_HANDLER_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_DIAGNOSTIC_JOB_HANDLER_H

#include "i_diagnostic_job_handler.h"
#include "score/mw/log/log_level.h"
#include "score/datarouter/include/dlt/dltid.h"

namespace score
{
namespace logging
{
namespace dltserver
{

class ReadLogChannelNamesHandler final : public IDiagnosticJobHandler
{
  public:
    ReadLogChannelNamesHandler() = default;
    std::string Execute(IDltLogServer& srv) override;
};

class ResetToDefaultHandler final : public IDiagnosticJobHandler
{
  public:
    ResetToDefaultHandler() = default;
    std::string Execute(IDltLogServer& srv) override;
};

class StoreDltConfigHandler final : public IDiagnosticJobHandler
{
  public:
    StoreDltConfigHandler() = default;
    std::string Execute(IDltLogServer& srv) override;
};

class SetTraceStateHandler final : public IDiagnosticJobHandler
{
  public:
    SetTraceStateHandler() = default;
    std::string Execute(IDltLogServer& srv) override;
};

class SetDefaultTraceStateHandler final : public IDiagnosticJobHandler
{
  public:
    SetDefaultTraceStateHandler() = default;
    std::string Execute(IDltLogServer& srv) override;
};

class SetLogChannelThresholdHandler final : public IDiagnosticJobHandler
{
  private:
    score::platform::DltidT channel_;
    LoglevelT threshold_;

  public:
    SetLogChannelThresholdHandler(score::platform::DltidT channel, LoglevelT threshold)
        : channel_(channel), threshold_(threshold)
    {
    }

    bool operator==(const SetLogChannelThresholdHandler& rhs) const noexcept
    {
        return channel_ == rhs.channel_ && threshold_ == rhs.threshold_;
    }

    std::string Execute(IDltLogServer& srv) override;
};

class SetLogLevelHandler final : public IDiagnosticJobHandler
{
  private:
    score::platform::DltidT app_id_;
    score::platform::DltidT ctx_id_;
    ThresholdT threshold_;

  public:
    SetLogLevelHandler(score::platform::DltidT app_id, score::platform::DltidT ctx_id, ThresholdT threshold)
        : app_id_(app_id), ctx_id_(ctx_id), threshold_(threshold)
    {
    }

    bool operator==(const SetLogLevelHandler& rhs) const noexcept
    {
        return app_id_ == rhs.app_id_ && ctx_id_ == rhs.ctx_id_ && threshold_ == rhs.threshold_;
    }

    std::string Execute(IDltLogServer& srv) override;
};

class SetMessagingFilteringStateHandler final : public IDiagnosticJobHandler
{
  private:
    bool enabled_;

  public:
    explicit SetMessagingFilteringStateHandler(bool enabled) : enabled_(enabled) {}

    bool operator==(const SetMessagingFilteringStateHandler& rhs) const noexcept
    {
        return enabled_ == rhs.enabled_;
    }

    std::string Execute(IDltLogServer& srv) override;
};

class SetDefaultLogLevelHandler final : public IDiagnosticJobHandler
{
  private:
    LoglevelT level_;

  public:
    explicit SetDefaultLogLevelHandler(LoglevelT level) : level_(level) {}

    bool operator==(const SetDefaultLogLevelHandler& rhs) const noexcept
    {
        return level_ == rhs.level_;
    }

    std::string Execute(IDltLogServer& srv) override;
};

class SetLogChannelAssignmentHandler final : public IDiagnosticJobHandler
{
  private:
    score::platform::DltidT app_id_;
    score::platform::DltidT ctx_id_;
    score::platform::DltidT channel_;
    AssignmentAction assignment_flag_;

  public:
    SetLogChannelAssignmentHandler(score::platform::DltidT app_id,
                                   score::platform::DltidT ctx_id,
                                   score::platform::DltidT channel,
                                   AssignmentAction assignment_flag)
        : app_id_(app_id), ctx_id_(ctx_id), channel_(channel), assignment_flag_(assignment_flag)
    {
    }

    bool operator==(const SetLogChannelAssignmentHandler& rhs) const noexcept
    {
        return app_id_ == rhs.app_id_ && ctx_id_ == rhs.ctx_id_ && channel_ == rhs.channel_ &&
               assignment_flag_ == rhs.assignment_flag_;
    }

    std::string Execute(IDltLogServer& srv) override;
};

class SetDltOutputEnableHandler final : public IDiagnosticJobHandler
{
  private:
    bool enable_;

  public:
    explicit SetDltOutputEnableHandler(bool enable) : enable_(enable) {}

    bool operator==(const SetDltOutputEnableHandler& rhs) const noexcept
    {
        return enable_ == rhs.enable_;
    }

    std::string Execute(IDltLogServer& srv) override;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_DIAGNOSTIC_JOB_HANDLER_H
