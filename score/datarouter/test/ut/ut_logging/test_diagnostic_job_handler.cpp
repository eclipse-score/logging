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
#include "score/datarouter/include/daemon/diagnostic_job_parser.h"
#include "score/datarouter/mocks/daemon/diagnostic_job_parser_mock.h"
#include "score/datarouter/mocks/daemon/dlt_log_server_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;

namespace score
{
namespace logging
{
namespace dltserver
{

class DiagnosticJobHandlerTest : public ::testing::Test
{
  public:
    mock::DltLogServerMock dltlogserver_mock_;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

using namespace score::logging::dltserver;

namespace test
{

TEST_F(DiagnosticJobHandlerTest, ReadLogChannelNamesHandler_OK)
{
    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<ReadLogChannelNamesHandler>();

    EXPECT_CALL(dltlogserver_mock_, ReadLogChannelNames()).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, ResetToDefaultHandler_OK)
{
    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<ResetToDefaultHandler>();

    EXPECT_CALL(dltlogserver_mock_, ResetToDefault()).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, StoreDltConfigHandler_OK)
{
    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<StoreDltConfigHandler>();

    EXPECT_CALL(dltlogserver_mock_, StoreDltConfig()).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetTraceStateHandler_OK)
{
    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<SetTraceStateHandler>();

    EXPECT_CALL(dltlogserver_mock_, SetTraceState()).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetDefaultTraceStateHandler_OK)
{
    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<SetDefaultTraceStateHandler>();

    EXPECT_CALL(dltlogserver_mock_, SetDefaultTraceState()).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetLogChannelThresholdHandler_OK)
{
    loglevel_t threshold = loglevel_t::kFatal;
    score::platform::dltid_t channel = extractId("1", 1);

    std::unique_ptr<IDiagnosticJobHandler> handler =
        std::make_unique<SetLogChannelThresholdHandler>(channel, threshold);

    EXPECT_CALL(dltlogserver_mock_, SetLogChannelThreshold(channel, threshold)).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetLogLevelHandler_OK)
{
    score::platform::dltid_t appId = extractId("1", 1);
    score::platform::dltid_t ctxId = extractId("2", 1);
    threshold_t threshold = ThresholdCmd::UseDefault;

    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<SetLogLevelHandler>(appId, ctxId, threshold);

    EXPECT_CALL(dltlogserver_mock_, SetLogLevel(appId, ctxId, threshold)).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetMessagingFilteringStateHandler_OK)
{
    bool enabled = 0;

    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<SetMessagingFilteringStateHandler>(enabled);

    EXPECT_CALL(dltlogserver_mock_, SetMessagingFilteringState(enabled)).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetDefaultLogLevelHandler_OK)
{
    loglevel_t threshold = loglevel_t::kFatal;

    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<SetDefaultLogLevelHandler>(threshold);

    EXPECT_CALL(dltlogserver_mock_, SetDefaultLogLevel(threshold)).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetLogChannelAssignmentHandler_OK)
{
    score::platform::dltid_t appId = extractId("1", 1);
    score::platform::dltid_t ctxId = extractId("2", 1);
    score::platform::dltid_t channel = extractId("2", 1);

    AssignmentAction assignment_flag = AssignmentAction::Add;

    std::unique_ptr<IDiagnosticJobHandler> handler =
        std::make_unique<SetLogChannelAssignmentHandler>(appId, ctxId, channel, assignment_flag);

    EXPECT_CALL(dltlogserver_mock_, SetLogChannelAssignment(appId, ctxId, channel, assignment_flag)).Times(1);

    handler->execute(dltlogserver_mock_);
}

TEST_F(DiagnosticJobHandlerTest, SetDltOutputEnableHandler_OK)
{
    auto flag = config::DISABLE;

    std::unique_ptr<IDiagnosticJobHandler> handler = std::make_unique<SetDltOutputEnableHandler>(flag);

    EXPECT_CALL(dltlogserver_mock_, SetDltOutputEnable(flag)).Times(1);

    handler->execute(dltlogserver_mock_);
}

}  // namespace test
