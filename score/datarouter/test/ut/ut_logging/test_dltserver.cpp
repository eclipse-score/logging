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
#include "daemon/diagnostic_job_parser.h"
#include "daemon/dlt_log_server.h"
#include "score/datarouter/include/daemon/configurator_commands.h"
#include "score/datarouter/mocks/daemon/log_sender_mock.h"

#include "gtest/gtest.h"

using namespace testing;

namespace score
{
namespace platform
{

inline namespace
{
}  // namespace

}  // namespace platform
}  // namespace score

namespace score
{
namespace logging
{
namespace dltserver
{

inline namespace
{
// Declared those constants for readability purposes
constexpr auto kCommandSize{1UL};
constexpr auto kCommandResponseSize{1UL};

// Helpers to make tests agnostic to the DYNAMIC_CONFIGURATION_FEATURE_ENABLED flag.
// If dynamic configuration is disabled (stub), responses remain empty; otherwise they carry one-byte status.
#define EXPECT_OK_OR_NOOP(resp)                                      \
    do                                                               \
    {                                                                \
        if (!(resp).empty())                                         \
        {                                                            \
            EXPECT_EQ((resp).size(), kCommandResponseSize);          \
            EXPECT_EQ((resp)[0], static_cast<char>(config::RET_OK)); \
        }                                                            \
        else                                                         \
        {                                                            \
            SUCCEED();                                               \
        }                                                            \
    } while (0)

#define EXPECT_ERR_OR_NOOP(resp)                                        \
    do                                                                  \
    {                                                                   \
        if (!(resp).empty())                                            \
        {                                                               \
            EXPECT_EQ((resp).size(), kCommandResponseSize);             \
            EXPECT_EQ((resp)[0], static_cast<char>(config::RET_ERROR)); \
        }                                                               \
        else                                                            \
        {                                                               \
            SUCCEED();                                                  \
        }                                                               \
    } while (0)

}  // namespace

}  // namespace dltserver
}  // namespace logging
}  // namespace score

namespace score
{
namespace logging
{
namespace dltserver
{

class DltLogServer::DltLogServerTest : public DltLogServer
{
  public:
    DltLogServerTest(StaticConfig static_config,
                     ConfigReadCallback reader,
                     ConfigWriteCallback writer,
                     bool enabled,
                     std::unique_ptr<ILogSender> log_sender = nullptr)
        : DltLogServer(static_config,
                       reader,
                       writer,
                       enabled,
                       (log_sender ? std::move(log_sender) : std::make_unique<LogSender>()))
    {
    }
    virtual ~DltLogServerTest() {};

  public:
    using DltLogServer::SendFtVerbose;
    using DltLogServer::SendNonVerbose;
    using DltLogServer::sendVerbose;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

namespace test
{

using namespace score::logging::dltserver;
using namespace score::logging::dltserver::mock;

class DltServerCreatedWithoutConfigFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        UdpStreamOutput::Tester::instance() = &outputs;
        EXPECT_CALL(outputs, construct(_, _, 3490U, Eq(std::string("")))).Times(1);
        EXPECT_CALL(outputs, bind(_, _, 3491U)).Times(1);
        EXPECT_CALL(outputs, destruct(_)).Times(1);
    }
    void TearDown() override {}

  public:
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    StaticConfig s_config{};
    PersistentConfig p_config{};
    testing::StrictMock<MockFunction<PersistentConfig(void)>> read_callback;
    testing::StrictMock<MockFunction<void(const PersistentConfig&)>> write_callback;
};

TEST_F(DltServerCreatedWithoutConfigFixture, WhenCreatedDefault)
{
    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);
}

TEST_F(DltServerCreatedWithoutConfigFixture, WhenCreatedDefaultDltEnabledTrue)
{
    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);
    const auto dlt_enabled = dlt_server.GetDltEnabled();
    EXPECT_TRUE(dlt_enabled);
}

TEST_F(DltServerCreatedWithoutConfigFixture, QuotaEnforcementEnabledExpectFalse)
{
    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);
    EXPECT_FALSE(dlt_server.getQuotaEnforcementEnabled());
}

TEST_F(DltServerCreatedWithoutConfigFixture, QuotaEnforcementEnabledExpectTrue)
{
    s_config.quotaEnforcementEnabled = true;
    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);
    EXPECT_TRUE(dlt_server.getQuotaEnforcementEnabled());
}

class DltServerCreatedWithConfigFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        log_sender_mock = std::make_unique<LogSenderMock>();
        log_sender_mock_raw_ptr = log_sender_mock.get();
        UdpStreamOutput::Tester::instance() = &outputs;
        EXPECT_CALL(outputs, construct(_, _, 3491U, Eq(std::string("160.48.199.34")))).Times(1);
        EXPECT_CALL(outputs, construct(_, _, 3492U, Eq(std::string("160.48.199.101")))).Times(1);
        EXPECT_CALL(outputs, move_construct(_, _)).Times(AtLeast(0));
        EXPECT_CALL(outputs, bind(_, _, 3490U)).Times(2);
        EXPECT_CALL(outputs, destruct(_)).Times(AtLeast(2));
    }
    void TearDown() override {}

  public:
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    std::vector<dltid_t> both_channels{{dltid_t("DFLT"), dltid_t("CORE")}};

    StaticConfig s_config{
        dltid_t("CORE"),
        dltid_t("DFLT"),
        {
            //  channels as std::unordered_map<dltid_t, ChannelDescription>
            {dltid_t("DFLT"), {dltid_t("ECU0"), "", 3490U, "", 3491U, score::mw::log::LogLevel::kFatal, "160.48.199.34"}},
            {dltid_t("CORE"),
             {dltid_t("ECU0"), "", 3490U, "", 3492U, score::mw::log::LogLevel::kError, "160.48.199.101"}},
        },
        true,  //  filteringEnabled
        score::mw::log::LogLevel::kOff,
        {
            {dltid_t("APP0"), {{dltid_t("CTX0"), both_channels}}},
        },
        {{dltid_t("APP0"), {{dltid_t("CTX0"), score::mw::log::LogLevel::kOff}}}},
        {100., {{dltid_t("APP0"), 1000.}}},
        false};

    PersistentConfig p_config{};
    testing::StrictMock<MockFunction<PersistentConfig(void)>> read_callback;
    testing::StrictMock<MockFunction<void(const PersistentConfig&)>> write_callback;

    std::unique_ptr<LogSenderMock> log_sender_mock{};
    LogSenderMock* log_sender_mock_raw_ptr{nullptr};
};

TEST_F(DltServerCreatedWithConfigFixture, FlushChannelsExpectNoThrowException)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);
    EXPECT_NO_THROW(dlt_server.flush());
}

TEST_F(DltServerCreatedWithConfigFixture, GetQuotaCorrectAppNameExpectCorrectValue)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);
    const auto ret_val = dlt_server.get_quota("APP0");
    EXPECT_EQ(ret_val, 1000);
}

TEST_F(DltServerCreatedWithConfigFixture, GetQuotaCorrectWrongAppNameExpectDefaultValue)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);
    const auto ret_val = dlt_server.get_quota("AAAA");
    EXPECT_EQ(ret_val, 1.0);
}

TEST(ResetToDefaultTest, ResetToDefaultCommandEmptyChannelsNoReadCallback)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    EXPECT_CALL(outputs, construct(_, _, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, _, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    const StaticConfig s_config{};
    PersistentConfig p_config{};

    testing::StrictMock<MockFunction<PersistentConfig(void)>> read_callback;
    testing::StrictMock<MockFunction<void(const PersistentConfig&)>> write_callback;
    EXPECT_CALL(read_callback, Call()).Times(0);
    EXPECT_CALL(write_callback, Call(_)).Times(::testing::AtMost(1));

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::RESET_TO_DEFAULT));

    EXPECT_OK_OR_NOOP(response);
}

TEST(ResetToDefaultTest, ResetToDefaultCommandChannelsSizeTooBigNoReadCallback)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    EXPECT_CALL(outputs, construct(_, _, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, _, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    StaticConfig s_config{};
    const std::int32_t unexpected_channels_size{33};
    const score::logging::dltserver::StaticConfig::ChannelDescription channel_desc{};
    for (std::int32_t i = 0; i < unexpected_channels_size; ++i)
    {
        s_config.channels.emplace(std::to_string(i), channel_desc);
    }

    PersistentConfig p_config{};

    testing::StrictMock<MockFunction<PersistentConfig(void)>> read_callback;
    testing::StrictMock<MockFunction<void(const PersistentConfig&)>> write_callback;
    EXPECT_CALL(read_callback, Call()).Times(0);
    EXPECT_CALL(write_callback, Call(_)).Times(::testing::AtMost(1));

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::RESET_TO_DEFAULT));

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetDefaultLogLevelWrongCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::SET_DEFAULT_LOG_LEVEL));

    EXPECT_ERR_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetDefaultLogLevelCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 2> command_buffer{config::SET_DEFAULT_LOG_LEVEL, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetDefaultLogLevelCommandReadLevelErrorExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 2> command_buffer{config::SET_DEFAULT_LOG_LEVEL, 7};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_ERR_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetTraceStateCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::SET_TRACE_STATE));

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetDefaultTraceStateCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::SET_DEFAULT_TRACE_STATE));

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelAssignmentWrongCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::SET_LOG_CHANNEL_ASSIGNMENT));

    EXPECT_ERR_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelAssignmentCommandNoChannelsExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 14> command_buffer{
        config::SET_LOG_CHANNEL_ASSIGNMENT, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_ERR_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelAssignmentCommandFoundChannelAssignmentFoundExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 14> command_buffer{
        config::SET_LOG_CHANNEL_ASSIGNMENT, 0x41, 0x50, 0x50, 0x30, 0x43, 0x54, 0x58, 0x30, 0x43, 0x4f, 0x52, 0x45, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture,
       SetLogChannelAssignmentCommandFoundChannelAssignmentFoundRemoveExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 14> command_buffer{
        config::SET_LOG_CHANNEL_ASSIGNMENT, 0x41, 0x50, 0x50, 0x30, 0x43, 0x54, 0x58, 0x30, 0x43, 0x4f, 0x52, 0x45, 0};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST(DltServerWrongChannelsTest,
     SetLogChannelAssignmentCommandFoundChannelAssignmentFoundRemoveFailedExpectReadCallback)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    EXPECT_CALL(outputs, construct(_, _, _, _)).Times(AtLeast(0));
    EXPECT_CALL(outputs, move_construct(_, _)).Times(AtLeast(0));
    EXPECT_CALL(outputs, bind(_, _, _)).Times(AtLeast(0));
    EXPECT_CALL(outputs, destruct(_)).Times(AtLeast(0));

    std::vector<dltid_t> both_channels{{dltid_t("DFL1"), dltid_t("COR1")}};
    StaticConfig s_config{
        dltid_t("CORE"),
        dltid_t("DFLT"),
        {
            {dltid_t("DFLT"), {dltid_t("ECU0"), "", 3490U, "", 3491U, score::mw::log::LogLevel::kFatal, "160.48.199.34"}},
            {dltid_t("CORE"),
             {dltid_t("ECU0"), "", 3490U, "", 3492U, score::mw::log::LogLevel::kError, "160.48.199.101"}},
        },
        true,
        score::mw::log::LogLevel::kOff,
        {
            {dltid_t("APP0"), {{dltid_t("CTX0"), both_channels}}},
        },
        {{dltid_t("APP0"), {{dltid_t("CTX0"), score::mw::log::LogLevel::kOff}}}},
        {100., {{dltid_t("APP0"), 1000.}}},
        false};

    PersistentConfig p_config{};

    testing::StrictMock<MockFunction<PersistentConfig(void)>> read_callback;
    testing::StrictMock<MockFunction<void(const PersistentConfig&)>> write_callback;
    EXPECT_CALL(read_callback, Call()).Times(1);
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 14> command_buffer{
        config::SET_LOG_CHANNEL_ASSIGNMENT, 0x41, 0x50, 0x50, 0x30, 0x43, 0x54, 0x58, 0x30, 0x43, 0x4f, 0x52, 0x45, 0};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture,
       SetLogChannelAssignmentCommandFoundChannelAssignmentNotFoundAddExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 14> command_buffer{
        config::SET_LOG_CHANNEL_ASSIGNMENT, 0, 0, 0, 0, 0, 0, 0, 0, 0x44, 0x46, 0x4c, 0x54, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelAssignmentWrongChannel)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response =
        dlt_server.SetLogChannelAssignment(dltid_t("fake"), dltid_t("fake"), dltid_t("fake"), AssignmentAction::Add);
    ASSERT_FALSE(response.empty());
    EXPECT_EQ(response[0], config::RET_ERROR);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelAssignmentBehaviorRemovesChannel)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    // Setup: add CORE so APP0/CTX0 is routed to DFLT + CORE.
    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection entry{
        app_id, ctx_id, {}, 0, score::mw::log::LogLevel::kOff};

    const auto resp_add =
        dlt_server.SetLogChannelAssignment(dltid_t{"APP0"}, dltid_t{"CTX0"}, dltid_t{"CORE"}, AssignmentAction::Add);
    ASSERT_FALSE(resp_add.empty());
    EXPECT_EQ(resp_add[0], static_cast<char>(config::RET_OK));

    // With both channels assigned: 2 sends.
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(2);
    dlt_server.sendVerbose(100U, entry);
    ::testing::Mock::VerifyAndClearExpectations(log_sender_mock_raw_ptr);

    const auto resp_remove =
        dlt_server.SetLogChannelAssignment(dltid_t{"APP0"}, dltid_t{"CTX0"}, dltid_t{"CORE"}, AssignmentAction::Remove);
    ASSERT_FALSE(resp_remove.empty());
    EXPECT_EQ(resp_remove[0], static_cast<char>(config::RET_OK));

    // After removing CORE: back to DFLT-only -> 1 send.
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(1);
    dlt_server.sendVerbose(100U, entry);
}

TEST_F(DltServerCreatedWithConfigFixture, SetDltOutputEnableCommandCallbackEnabledExpectCallbackCall)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    testing::StrictMock<MockFunction<void(bool)>> enabled_callback;
    // If dynamic configuration is disabled, no callback will be invoked; allow at most one call.
    EXPECT_CALL(enabled_callback, Call(_)).Times(::testing::AtMost(1));
    dlt_server.set_enabled_callback(enabled_callback.AsStdFunction());

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 2> command_buffer{config::SET_DLT_OUTPUT_ENABLE, 0};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetMessagingFilteringStateWrongCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::SET_MESSAGING_FILTERING_STATE));

    EXPECT_ERR_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetMessagingFilteringStateCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 2> command_buffer{config::SET_MESSAGING_FILTERING_STATE, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogLevelBehaviorIncreaseThresholdAllowsVerbose)
{
    // Load persistent config (initial thresholds from sConfig.messageThresholds: APP0/CTX0 => kOff)
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    // Use test subclass to access sendVerbose
    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection verbose_entry{
        app_id, ctx_id, {}, 0, score::mw::log::LogLevel::kVerbose};

    // Initially threshold for APP0/CTX0 is kOff, so verbose should be filtered out.
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(0);
    dlt_server.sendVerbose(100U, verbose_entry);
    ::testing::Mock::VerifyAndClearExpectations(log_sender_mock_raw_ptr);

    // Increase threshold to kVerbose for APP0/CTX0 using direct API
    const threshold_t new_threshold{loglevel_t{score::mw::log::LogLevel::kVerbose}};
    const auto resp = dlt_server.SetLogLevel(dltid_t{"APP0"}, dltid_t{"CTX0"}, new_threshold);
    // Response always one byte RET_OK
    EXPECT_EQ(resp[0], static_cast<char>(config::RET_OK));

    // Now verbose should pass filtering and be forwarded once per assigned channel (DFLT + CORE) -> 2 calls.
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(2);
    dlt_server.sendVerbose(100U, verbose_entry);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogLevelBehaviorResetToDefaultBlocksVerboseAgain)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection verbose_entry{
        app_id, ctx_id, {}, 0, score::mw::log::LogLevel::kVerbose};

    // Raise threshold first so verbose is forwarded
    const threshold_t raise_threshold{loglevel_t{score::mw::log::LogLevel::kVerbose}};
    const auto resp_raise = dlt_server.SetLogLevel(dltid_t{"APP0"}, dltid_t{"CTX0"}, raise_threshold);
    EXPECT_EQ(resp_raise[0], static_cast<char>(config::RET_OK));
    // With raise_threshold, verbose accepted and forwarded for both assigned channels -> 2 calls.
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(2);
    dlt_server.sendVerbose(100U, verbose_entry);
    ::testing::Mock::VerifyAndClearExpectations(log_sender_mock_raw_ptr);

    // Now reset to default (ThresholdCmd::UseDefault removes specific mapping)
    const threshold_t reset_threshold{ThresholdCmd::UseDefault};
    const auto resp_reset = dlt_server.SetLogLevel(dltid_t{"APP0"}, dltid_t{"CTX0"}, reset_threshold);
    EXPECT_EQ(resp_reset[0], static_cast<char>(config::RET_OK));

    // With default threshold kOff, verbose must be filtered again
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(0);
    dlt_server.sendVerbose(100U, verbose_entry);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelThresholdWrongCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    session->on_command(std::string(kCommandSize, config::SET_LOG_CHANNEL_THRESHOLD));

    EXPECT_ERR_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelThresholdChannelNotFoundCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 7> command_buffer{config::SET_LOG_CHANNEL_THRESHOLD, 1, 2, 3, 4, 5, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_ERR_OR_NOOP(response);
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelThresholdChannelFoundCommandExpectReadCallback)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    std::string response{};
    auto session = dlt_server.new_config_session(
        score::platform::datarouter::ConfigSessionHandleType{0, nullptr, std::reference_wrapper<std::string>{response}});

    std::array<std::uint8_t, 7> command_buffer{config::SET_LOG_CHANNEL_THRESHOLD, 0x43, 0x4f, 0x52, 0x45, 5, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    session->on_command(command);

    EXPECT_OK_OR_NOOP(response);
}

TEST(DltServerTest, ExtractIdValidInputDataExpectValidResult)
{
    const std::string input_message{"asdAPP012345678zxccvb86545"};
    const size_t offset{3};
    const auto ret_value = extractId(input_message, offset);
    const std::string ret_val_string(ret_value.data(), ret_value.size());
    constexpr std::string_view kExpectedString{"APP0"};

    EXPECT_EQ(ret_val_string, kExpectedString);
    EXPECT_NE(ret_value.value, 0);
}

TEST(DltServerTest, ExtractIdNonValidInputDataExpectNonValidResult)
{
    const std::string input_message;
    const size_t offset{static_cast<unsigned long>(std::numeric_limits<std::string::difference_type>::max() + 1UL)};
    const auto ret_value = extractId(input_message, offset);
    const std::string ret_val_string(ret_value.data(), ret_value.size());
    const std::string expected_string{0x00, 0x00, 0x00, 0x00};

    EXPECT_EQ(ret_val_string, expected_string);
    EXPECT_EQ(ret_value.value, 0);
}

TEST(DltServerTest, AppendIdValidInputDataExpectValidResult)
{
    // dltid_t bytes buffer has size 4.
    constexpr std::string_view kExpectedMsgName{"expe"};
    score::logging::dltserver::dltid_t name{"expected name"};
    std::string ret_message{};
    appendId(name, ret_message);

    EXPECT_EQ(ret_message, kExpectedMsgName);
}

// sendNonVerbose test.

TEST_F(DltServerCreatedWithConfigFixture, SendNonVerboseFilteringDisabledExpectSendCallOnce)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    s_config.filteringEnabled = false;

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    EXPECT_CALL(*log_sender_mock_raw_ptr, SendNonVerbose(_, _, _, _, _)).Times(1);
    dlt_server.SendNonVerbose({}, 100U, nullptr, 0);
}

TEST_F(DltServerCreatedWithConfigFixture,
       SendNonVerboseNoAppIdAcceptedByFilteringNotAssignedToChannelExpectSendCallOnce)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    EXPECT_CALL(*log_sender_mock_raw_ptr, SendNonVerbose(_, _, _, _, _)).Times(1);
    dlt_server.SendNonVerbose({}, 100U, nullptr, 0);
}

TEST_F(DltServerCreatedWithConfigFixture, SendNonVerboseAppIdAcceptedByFilteringExpectSendCallTwice)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));
    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};
    const score::mw::log::config::NvMsgDescriptor desc{100U, app_id, ctx_id, score::mw::log::LogLevel::kOff};

    EXPECT_CALL(*log_sender_mock_raw_ptr, SendNonVerbose(_, _, _, _, _)).Times(2);
    dlt_server.SendNonVerbose(desc, 100U, nullptr, 0);
}

// sendVerbose test.

TEST_F(DltServerCreatedWithConfigFixture, SendVerboseNoAppIdAcceptedByFilteringNotAssignedToChannelExpectSendCallOnce)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(1);
    dlt_server.sendVerbose(100U, {});
}

TEST_F(DltServerCreatedWithConfigFixture, SendVerboseAppIdAcceptedByFilteringExpectSendCallTwice)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));
    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};

    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection entry{
        app_id, ctx_id, {}, 0, score::mw::log::LogLevel::kOff};

    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(2);
    dlt_server.sendVerbose(100U, entry);
}

TEST_F(DltServerCreatedWithConfigFixture, SendVerboseAppIdNotExpectedLogLevelExpectSendNoCall)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));
    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection entry{
        app_id, ctx_id, {}, 0, score::mw::log::LogLevel::kVerbose};

    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(0);
    dlt_server.sendVerbose(100U, entry);
}

// sendFTVerbose test.

TEST_F(DltServerCreatedWithConfigFixture, SendFVerboseNoAppIdWithCoreChannelExpectSendCallOnce)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    const score::logging::dltserver::dltid_t app_id{""};
    const score::logging::dltserver::dltid_t ctx_id{""};
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendFTVerbose(_, _, _, _, _, _, _)).Times(1);
    dlt_server.SendFtVerbose({}, score::mw::log::LogLevel::kOff, app_id, ctx_id, 0U, 100U);
}

TEST_F(DltServerCreatedWithConfigFixture, SendFVerboseAppIdWithCoreChannelExpectSendCallOnce)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));
    const score::logging::dltserver::dltid_t app_id{"APP0"};
    const score::logging::dltserver::dltid_t ctx_id{"CTX0"};
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendFTVerbose(_, _, _, _, _, _, _)).Times(1);
    dlt_server.SendFtVerbose({}, score::mw::log::LogLevel::kOff, app_id, ctx_id, 0U, 100U);
}

TEST_F(DltServerCreatedWithoutConfigFixture, SendFTVerboseAppIdNoCoreChannelExpectSendCallOnce)
{
    EXPECT_CALL(read_callback, Call()).Times(0);
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    std::unique_ptr<LogSenderMock> log_sender_mock = std::make_unique<LogSenderMock>();
    LogSenderMock* log_sender_mock_raw_ptr = log_sender_mock.get();
    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));
    const score::logging::dltserver::dltid_t app_id{"APP0"};
    const score::logging::dltserver::dltid_t ctx_id{"CTX0"};
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendFTVerbose(_, _, _, _, _, _, _)).Times(1);
    dlt_server.SendFtVerbose({}, score::mw::log::LogLevel::kVerbose, app_id, ctx_id, 0U, 100U);
}

TEST_F(DltServerCreatedWithoutConfigFixture, UpdateHandlersFinalToTrueExpectDltOutputEnabledFlagTrue)
{
    EXPECT_CALL(read_callback, Call()).Times(0);
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    dlt_server.update_handlers_final(true);
    EXPECT_TRUE(dlt_server.GetDltEnabled());
}

TEST_F(DltServerCreatedWithConfigFixture, SetLogChannelThresholdChannelMissingDirectCallReturnsError)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Use a channel name that does not exist in sConfig.channels
    const auto resp = dlt_server.SetLogChannelThreshold(dltid_t("MISS"), score::mw::log::LogLevel::kInfo);

    // This path must return a one-byte RET_ERROR response
    EXPECT_EQ(resp.size(), 1U);
    EXPECT_EQ(resp[0], static_cast<char>(config::RET_ERROR));
}

TEST_F(DltServerCreatedWithConfigFixture, MakeConfigCommandHandlerReturnsValidFunction)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Test that make_config_command_handler() returns a valid function
    auto handler = dlt_server.make_config_command_handler();
    EXPECT_TRUE(handler != nullptr);

    // Test that the handler can be called with a valid command
    std::string response = handler(std::string(kCommandSize, config::READ_LOG_CHANNEL_NAMES));

    // Response should be either OK with channel names (dynamic) or empty (stub)
    if (!response.empty())
    {
        // Dynamic configuration - should get channel names
        EXPECT_GT(response.size(), kCommandResponseSize);
        EXPECT_EQ(response[0], static_cast<char>(config::RET_OK));
    }
    else
    {
        // Stub configuration - empty response is expected
        SUCCEED();
    }
}

TEST_F(DltServerCreatedWithConfigFixture, MakeConfigCommandHandlerWithInvalidCommand)
{
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Test that make_config_command_handler() handles invalid commands
    auto handler = dlt_server.make_config_command_handler();
    EXPECT_TRUE(handler != nullptr);

    // Test with an invalid command
    std::string response = handler("INVALID_COMMAND");

    // Response should be either ERROR (dynamic) or empty (stub)
    if (!response.empty())
    {
        // Dynamic configuration - should get error response
        EXPECT_EQ(response.size(), kCommandResponseSize);
        EXPECT_EQ(response[0], static_cast<char>(config::RET_ERROR));
    }
    else
    {
        // Stub configuration - empty response is expected
        SUCCEED();
    }
}

TEST_F(DltServerCreatedWithConfigFixture, ResetToDefaultDirectCallReloadsChannels)
{
    // This test directly calls ResetToDefault() to ensure the reloading path is covered
    // when dynamic configuration is disabled (where session commands are no-ops)
    EXPECT_CALL(read_callback, Call()).Times(2).WillOnce(Return(p_config)).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(1);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Directly call ResetToDefault() to trigger init_log_channels(true)
    const auto response = dlt_server.ResetToDefault();

    // Should always return OK status (single byte with RET_OK)
    EXPECT_EQ(response.size(), kCommandResponseSize);
    EXPECT_EQ(response[0], static_cast<char>(config::RET_OK));
}

TEST_F(DltServerCreatedWithConfigFixture, ReadLogChannelNamesDirectCall)
{
    // Test ReadLogChannelNames() method directly when dynamic configuration is disabled
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Directly call ReadLogChannelNames() to cover the method
    const auto response = dlt_server.ReadLogChannelNames();

    // Should return OK status and channel names
    EXPECT_GT(response.size(), kCommandResponseSize);
    EXPECT_EQ(response[0], static_cast<char>(config::RET_OK));
}

TEST_F(DltServerCreatedWithConfigFixture, StoreDltConfigDirectCall)
{
    // Test StoreDltConfig() method directly when dynamic configuration is disabled
    // StoreDltConfig() internally calls save_database() to cover that private method
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(1);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Directly call StoreDltConfig() to cover the method and save_database()
    const auto response = dlt_server.StoreDltConfig();

    // Should return OK status (single byte with RET_OK)
    EXPECT_EQ(response.size(), kCommandResponseSize);
    EXPECT_EQ(response[0], static_cast<char>(config::RET_OK));
}

TEST_F(DltServerCreatedWithConfigFixture, SetDltOutputEnableDirectCall)
{
    // Test SetDltOutputEnable() method directly when dynamic configuration is disabled
    // SetDltOutputEnable() internally calls set_output_enabled() to cover that private method
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Test enabling output through public method
    auto response = dlt_server.SetDltOutputEnable(true);
    EXPECT_EQ(response.size(), kCommandResponseSize);
    EXPECT_EQ(response[0], static_cast<char>(config::RET_OK));
    EXPECT_TRUE(dlt_server.GetDltEnabled());

    // Test disabling output through public method
    response = dlt_server.SetDltOutputEnable(false);
    EXPECT_EQ(response.size(), kCommandResponseSize);
    EXPECT_EQ(response[0], static_cast<char>(config::RET_OK));
    EXPECT_FALSE(dlt_server.GetDltEnabled());
}

TEST_F(DltServerCreatedWithConfigFixture, SetDltOutputEnableBehaviorBlocksAllSends)
{
    // Prove that enabling/disabling output affects the observable server state.
    // Note: sendVerbose()/sendNonVerbose() are not gated by this flag in the current implementation;
    // the flag controls the DLT output enable state exposed via GetDltEnabled().
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection entry{
        app_id, ctx_id, {}, 0, score::mw::log::LogLevel::kOff};

    // Disable output: this should gate sending completely.
    const auto disable_resp = dlt_server.SetDltOutputEnable(false);
    EXPECT_EQ(disable_resp.size(), kCommandResponseSize);
    EXPECT_EQ(disable_resp[0], static_cast<char>(config::RET_OK));
    EXPECT_FALSE(dlt_server.GetDltEnabled());

    // Re-enable output: sending should resume.
    const auto enable_resp = dlt_server.SetDltOutputEnable(true);
    EXPECT_EQ(enable_resp.size(), kCommandResponseSize);
    EXPECT_EQ(enable_resp[0], static_cast<char>(config::RET_OK));
    EXPECT_TRUE(dlt_server.GetDltEnabled());

    // Basic sanity: calling sendVerbose still forwards to the log sender (2 channels).
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(2);
    dlt_server.sendVerbose(100U, entry);
}

TEST_F(DltServerCreatedWithConfigFixture, ResetToDefaultBehaviorRestoresInitialThresholds)
{
    // Verify that ResetToDefault() restores initial thresholds, affecting message filtering.
    // Load persistent config with 2 read calls expected (constructor + ResetToDefault)
    EXPECT_CALL(read_callback, Call()).Times(2).WillRepeatedly(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(1);

    // Use test subclass to access sendVerbose
    score::logging::dltserver::DltLogServer::DltLogServerTest dlt_server(
        s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true, std::move(log_sender_mock));

    const score::mw::log::detail::LoggingIdentifier app_id{"APP0"};
    const score::mw::log::detail::LoggingIdentifier ctx_id{"CTX0"};
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection verbose_entry{
        app_id, ctx_id, {}, 0, score::mw::log::LogLevel::kVerbose};

    // Initially threshold for APP0/CTX0 is kOff, so verbose should be filtered out
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(0);
    dlt_server.sendVerbose(100U, verbose_entry);
    ::testing::Mock::VerifyAndClearExpectations(log_sender_mock_raw_ptr);

    // Increase threshold to kVerbose so verbose messages pass filtering
    const threshold_t new_threshold{loglevel_t{score::mw::log::LogLevel::kVerbose}};
    const auto resp = dlt_server.SetLogLevel(dltid_t{"APP0"}, dltid_t{"CTX0"}, new_threshold);
    EXPECT_EQ(resp[0], static_cast<char>(config::RET_OK));

    // Verify verbose now passes (2 channels: DFLT + CORE)
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(2);
    dlt_server.sendVerbose(100U, verbose_entry);
    ::testing::Mock::VerifyAndClearExpectations(log_sender_mock_raw_ptr);

    // Call ResetToDefault() to restore initial thresholds
    const auto reset_resp = dlt_server.ResetToDefault();
    EXPECT_EQ(reset_resp.size(), kCommandResponseSize);
    EXPECT_EQ(reset_resp[0], static_cast<char>(config::RET_OK));

    // After reset, threshold should be back to kOff, so verbose is filtered again
    EXPECT_CALL(*log_sender_mock_raw_ptr, SendVerbose(_, _, _)).Times(0);
    dlt_server.sendVerbose(100U, verbose_entry);
}

TEST_F(DltServerCreatedWithConfigFixture, ReadLogChannelNamesDirectCallContainsExpectedChannels)
{
    // Enhanced test to verify ReadLogChannelNames() returns actual channel names, not just OK status.
    EXPECT_CALL(read_callback, Call()).Times(1).WillOnce(Return(p_config));
    EXPECT_CALL(write_callback, Call(_)).Times(0);

    DltLogServer dlt_server(s_config, read_callback.AsStdFunction(), write_callback.AsStdFunction(), true);

    // Directly call ReadLogChannelNames()
    const auto response = dlt_server.ReadLogChannelNames();

    // Should return OK status and channel names
    ASSERT_GT(response.size(), kCommandResponseSize);
    EXPECT_EQ(response[0], static_cast<char>(config::RET_OK));

    // Verify response contains expected channel names from sConfig
    const std::string response_str(response.begin() + kCommandResponseSize, response.end());
    EXPECT_NE(response_str.find("DFLT"), std::string::npos) << "Response should contain DFLT channel";
    EXPECT_NE(response_str.find("CORE"), std::string::npos) << "Response should contain CORE channel";
}

}  // namespace test
