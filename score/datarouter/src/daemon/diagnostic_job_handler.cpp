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

std::string ReadLogChannelNamesHandler::Execute(IDltLogServer& srv)
{
    return srv.ReadLogChannelNames();
}

std::string ResetToDefaultHandler::Execute(IDltLogServer& srv)
{
    return srv.ResetToDefault();
}

std::string StoreDltConfigHandler::Execute(IDltLogServer& srv)
{
    return srv.StoreDltConfig();
}

std::string SetTraceStateHandler::Execute(IDltLogServer& srv)
{
    return srv.SetTraceState();
}

std::string SetDefaultTraceStateHandler::Execute(IDltLogServer& srv)
{
    return srv.SetDefaultTraceState();
}

std::string SetLogChannelThresholdHandler::Execute(IDltLogServer& srv)
{
    return srv.SetLogChannelThreshold(channel_, threshold_);
}

std::string SetLogLevelHandler::Execute(IDltLogServer& srv)
{
    return srv.SetLogLevel(app_id_, ctx_id_, threshold_);
}

std::string SetMessagingFilteringStateHandler::Execute(IDltLogServer& srv)
{
    return srv.SetMessagingFilteringState(enabled_);
}

std::string SetDefaultLogLevelHandler::Execute(IDltLogServer& srv)
{
    return srv.SetDefaultLogLevel(level_);
}

std::string SetLogChannelAssignmentHandler::Execute(IDltLogServer& srv)
{
    return srv.SetLogChannelAssignment(app_id_, ctx_id_, channel_, assignment_flag_);
}

std::string SetDltOutputEnableHandler::Execute(IDltLogServer& srv)
{
    return srv.SetDltOutputEnable(enable_);
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
