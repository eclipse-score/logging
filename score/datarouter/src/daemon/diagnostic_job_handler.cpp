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

#include "score/datarouter/include/daemon/diagnostic_job_handler.h"
#include "score/datarouter/include/daemon/i_dlt_log_server.h"

namespace score
{
namespace logging
{
namespace dltserver
{

const std::string ReadLogChannelNamesHandler::execute(IDltLogServer& srv)
{
    return srv.ReadLogChannelNames();
}

const std::string ResetToDefaultHandler::execute(IDltLogServer& srv)
{
    return srv.ResetToDefault();
}

const std::string StoreDltConfigHandler::execute(IDltLogServer& srv)
{
    return srv.StoreDltConfig();
}

const std::string SetTraceStateHandler::execute(IDltLogServer& srv)
{
    return srv.SetTraceState();
}

const std::string SetDefaultTraceStateHandler::execute(IDltLogServer& srv)
{
    return srv.SetDefaultTraceState();
}

const std::string SetLogChannelThresholdHandler::execute(IDltLogServer& srv)
{
    return srv.SetLogChannelThreshold(channel_, threshold_);
}

const std::string SetLogLevelHandler::execute(IDltLogServer& srv)
{
    return srv.SetLogLevel(appId_, ctxId_, threshold_);
}

const std::string SetMessagingFilteringStateHandler::execute(IDltLogServer& srv)
{
    return srv.SetMessagingFilteringState(enabled_);
}

const std::string SetDefaultLogLevelHandler::execute(IDltLogServer& srv)
{
    return srv.SetDefaultLogLevel(level_);
}

const std::string SetLogChannelAssignmentHandler::execute(IDltLogServer& srv)
{
    return srv.SetLogChannelAssignment(appId_, ctxId_, channel_, assignment_flag_);
}

const std::string SetDltOutputEnableHandler::execute(IDltLogServer& srv)
{
    return srv.SetDltOutputEnable(enable_);
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
