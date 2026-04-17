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

#ifndef SCORE_DATAROUTER_MOCKS_DAEMON_DLT_LOG_SERVER_MOCK_H
#define SCORE_DATAROUTER_MOCKS_DAEMON_DLT_LOG_SERVER_MOCK_H

#include <gmock/gmock.h>

#include "score/datarouter/include/daemon/i_dlt_log_server.h"

namespace score
{
namespace logging
{
namespace dltserver
{

namespace mock
{
class DltLogServerMock : public IDltLogServer
{
  public:
    MOCK_METHOD(std::string, ReadLogChannelNames, (), (const, override));
    MOCK_METHOD(std::string, ResetToDefault, (), (override));
    MOCK_METHOD(std::string, StoreDltConfig, (), (override));
    MOCK_METHOD(std::string, SetTraceState, (), (override));
    MOCK_METHOD(std::string, SetDefaultTraceState, (), (override));
    MOCK_METHOD(std::string, SetLogChannelThreshold, (score::platform::DltidT, LoglevelT), (override));
    MOCK_METHOD(std::string, SetLogLevel, (score::platform::DltidT, score::platform::DltidT, ThresholdT), (override));
    MOCK_METHOD(std::string, SetMessagingFilteringState, (bool), (override));
    MOCK_METHOD(std::string, SetDefaultLogLevel, (LoglevelT), (override));
    MOCK_METHOD(std::string,
                SetLogChannelAssignment,
                (score::platform::DltidT, score::platform::DltidT, score::platform::DltidT, AssignmentAction),
                (override));
    MOCK_METHOD(std::string, SetDltOutputEnable, (bool), (override));
};

}  // namespace mock

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_MOCKS_DAEMON_DLT_LOG_SERVER_MOCK_H
