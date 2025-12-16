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

#include "applications/datarouter_feature_config.h"
#include "daemon/dlt_log_server.h"
#include "gmock/gmock.h"
#include "score/datarouter/include/daemon/configurator_commands.h"
#include "score/datarouter/mocks/daemon/udp_stream_output.h"

#include <gtest/gtest.h>

using namespace testing;

namespace test
{
using score::logging::dltserver::DltLogServer;
using score::logging::dltserver::PersistentConfig;
using score::logging::dltserver::StaticConfig;
using score::logging::dltserver::mock::UdpStreamOutput;

TEST(ConfigSessionFactoryUT, CreateSessionsAndHandleCommands)
{
    StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    EXPECT_CALL(outputs, construct(_, _, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, _, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    StaticConfig sConfig{};
    PersistentConfig pConfig{};
    StrictMock<MockFunction<PersistentConfig(void)>> readCallback;
    StrictMock<MockFunction<void(const PersistentConfig&)>> writeCallback;

    DltLogServer server(sConfig, readCallback.AsStdFunction(), writeCallback.AsStdFunction(), true);

    score::platform::datarouter::DynamicConfigurationHandlerFactoryType dynFactory;
    std::string respDyn;
    auto dynSession = dynFactory.CreateConfigSession(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{respDyn}},
        server.make_config_command_handler());
    ASSERT_NE(dynSession, nullptr);
    std::array<std::uint8_t, 2> badEnableDyn{score::logging::dltserver::config::SET_DLT_OUTPUT_ENABLE, 2};
    dynSession->on_command(std::string{badEnableDyn.begin(), badEnableDyn.end()});
    if (!respDyn.empty())
    {
        EXPECT_EQ(respDyn.size(), 1U);
        EXPECT_EQ(respDyn[0], static_cast<char>(score::logging::dltserver::config::RET_ERROR));
    }
}

}  // namespace test
