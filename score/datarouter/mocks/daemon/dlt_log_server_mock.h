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

#ifndef DLT_LOG_SERVER_MOCK_H_
#define DLT_LOG_SERVER_MOCK_H_

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
    MOCK_METHOD(const std::string, ReadLogChannelNames, (), (override));
    MOCK_METHOD(const std::string, ResetToDefault, (), (override));
    MOCK_METHOD(const std::string, StoreDltConfig, (), (override));
    MOCK_METHOD(const std::string, SetTraceState, (), (override));
    MOCK_METHOD(const std::string, SetDefaultTraceState, (), (override));
    MOCK_METHOD(const std::string, SetLogChannelThreshold, (score::platform::dltid_t, loglevel_t), (override));
    MOCK_METHOD(const std::string,
                SetLogLevel,
                (score::platform::dltid_t, score::platform::dltid_t, threshold_t),
                (override));
    MOCK_METHOD(const std::string, SetMessagingFilteringState, (bool), (override));
    MOCK_METHOD(const std::string, SetDefaultLogLevel, (loglevel_t), (override));
    MOCK_METHOD(const std::string,
                SetLogChannelAssignment,
                (score::platform::dltid_t, score::platform::dltid_t, score::platform::dltid_t, AssignmentAction),
                (override));
    MOCK_METHOD(const std::string, SetDltOutputEnable, (bool), (override));
};

}  // namespace mock

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // DLT_LOG_SERVER_MOCK_H_
