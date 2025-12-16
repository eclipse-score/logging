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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_DIAGNOSTIC_JOB_HANDLER_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_DIAGNOSTIC_JOB_HANDLER_H

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
    const std::string execute(IDltLogServer& srv) override;
};

class ResetToDefaultHandler final : public IDiagnosticJobHandler
{
  public:
    ResetToDefaultHandler() = default;
    const std::string execute(IDltLogServer& srv) override;
};

class StoreDltConfigHandler final : public IDiagnosticJobHandler
{
  public:
    StoreDltConfigHandler() = default;
    const std::string execute(IDltLogServer& srv) override;
};

class SetTraceStateHandler final : public IDiagnosticJobHandler
{
  public:
    SetTraceStateHandler() = default;
    const std::string execute(IDltLogServer& srv) override;
};

class SetDefaultTraceStateHandler final : public IDiagnosticJobHandler
{
  public:
    SetDefaultTraceStateHandler() = default;
    const std::string execute(IDltLogServer& srv) override;
};

class SetLogChannelThresholdHandler final : public IDiagnosticJobHandler
{
  private:
    score::platform::dltid_t channel_;
    loglevel_t threshold_;

  public:
    SetLogChannelThresholdHandler(score::platform::dltid_t channel, loglevel_t threshold)
        : channel_(channel), threshold_(threshold)
    {
    }

    bool operator==(const SetLogChannelThresholdHandler& rhs) const noexcept
    {
        return channel_ == rhs.channel_ && threshold_ == rhs.threshold_;
    }

    const std::string execute(IDltLogServer& srv) override;
};

class SetLogLevelHandler final : public IDiagnosticJobHandler
{
  private:
    score::platform::dltid_t appId_;
    score::platform::dltid_t ctxId_;
    threshold_t threshold_;

  public:
    SetLogLevelHandler(score::platform::dltid_t appId, score::platform::dltid_t ctxId, threshold_t threshold)
        : appId_(appId), ctxId_(ctxId), threshold_(threshold)
    {
    }

    bool operator==(const SetLogLevelHandler& rhs) const noexcept
    {
        return appId_ == rhs.appId_ && ctxId_ == rhs.ctxId_ && threshold_ == rhs.threshold_;
    }

    const std::string execute(IDltLogServer& srv) override;
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

    const std::string execute(IDltLogServer& srv) override;
};

class SetDefaultLogLevelHandler final : public IDiagnosticJobHandler
{
  private:
    loglevel_t level_;

  public:
    explicit SetDefaultLogLevelHandler(loglevel_t level) : level_(level) {}

    bool operator==(const SetDefaultLogLevelHandler& rhs) const noexcept
    {
        return level_ == rhs.level_;
    }

    const std::string execute(IDltLogServer& srv) override;
};

class SetLogChannelAssignmentHandler final : public IDiagnosticJobHandler
{
  private:
    score::platform::dltid_t appId_;
    score::platform::dltid_t ctxId_;
    score::platform::dltid_t channel_;
    AssignmentAction assignment_flag_;

  public:
    SetLogChannelAssignmentHandler(score::platform::dltid_t appId,
                                   score::platform::dltid_t ctxId,
                                   score::platform::dltid_t channel,
                                   AssignmentAction assignment_flag)
        : appId_(appId), ctxId_(ctxId), channel_(channel), assignment_flag_(assignment_flag)
    {
    }

    bool operator==(const SetLogChannelAssignmentHandler& rhs) const noexcept
    {
        return appId_ == rhs.appId_ && ctxId_ == rhs.ctxId_ && channel_ == rhs.channel_ &&
               assignment_flag_ == rhs.assignment_flag_;
    }

    const std::string execute(IDltLogServer& srv) override;
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

    const std::string execute(IDltLogServer& srv) override;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_DIAGNOSTIC_JOB_HANDLER_H
