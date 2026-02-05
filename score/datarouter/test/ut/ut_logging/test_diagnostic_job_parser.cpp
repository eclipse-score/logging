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

#include "daemon/diagnostic_job_parser.h"
#include "score/datarouter/include/daemon/configurator_commands.h"
#include "score/datarouter/include/daemon/diagnostic_job_handler.h"

#include "gtest/gtest.h"

namespace score
{
namespace logging
{
namespace dltserver
{

template <typename ConcreteHandler>
// Casts the base ptr to its expected derived type. However, if it's not the expected derived type, it returns nullptr.
const ConcreteHandler* ConvertHandlerTypeTo(const std::unique_ptr<IDiagnosticJobHandler>& handler) noexcept
{
    static_assert(std::is_base_of<IDiagnosticJobHandler, ConcreteHandler>::value,
                  "Concrete handler shall be derived from handler base class");

    return dynamic_cast<const ConcreteHandler*>(handler.get());
}

class DiagnosticJobParserTest : public ::testing::Test
{
  public:
    DiagnosticJobParser uut;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

namespace test
{

using namespace score::logging::dltserver;

TEST_F(DiagnosticJobParserTest, ReadLogChannelNames_OK)
{
    auto correct_handler = std::make_unique<ReadLogChannelNamesHandler>();
    auto handler = uut.parse(std::string{char(config::READ_LOG_CHANNEL_NAMES)});
    EXPECT_NE(ConvertHandlerTypeTo<ReadLogChannelNamesHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, ResetToDefault_OK)
{
    auto correct_handler = std::make_unique<ResetToDefaultHandler>();
    auto handler = uut.parse(std::string{char(config::RESET_TO_DEFAULT)});
    EXPECT_NE(ConvertHandlerTypeTo<ResetToDefaultHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, StoreDltConfig_OK)
{
    auto correct_handler = std::make_unique<StoreDltConfigHandler>();
    auto handler = uut.parse(std::string{char(config::STORE_DLT_CONFIG)});
    EXPECT_NE(ConvertHandlerTypeTo<StoreDltConfigHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, SetTraceState_OK)
{
    auto correct_handler = std::make_unique<SetTraceStateHandler>();
    auto handler = uut.parse(std::string{char(config::SET_TRACE_STATE)});
    EXPECT_NE(ConvertHandlerTypeTo<SetTraceStateHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, SetDefaultTraceState_OK)
{
    auto correct_handler = std::make_unique<SetDefaultTraceStateHandler>();
    auto handler = uut.parse(std::string{char(config::SET_DEFAULT_TRACE_STATE)});
    EXPECT_NE(ConvertHandlerTypeTo<SetDefaultTraceStateHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, SetLogChannelThreshold_OK)
{
    score::platform::dltid_t id("CORE");
    auto correct_handler = std::make_unique<SetLogChannelThresholdHandler>(id, score::mw::log::LogLevel::kDebug);
    std::array<std::uint8_t, 7> command_buffer{config::SET_LOG_CHANNEL_THRESHOLD, 0x43, 0x4f, 0x52, 0x45, 5, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogChannelThresholdHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetLogLevel_OK_UseDefault)
{
    score::platform::dltid_t id_1("CORE");
    score::platform::dltid_t id_2("APP0");
    auto correct_handler = std::make_unique<SetLogLevelHandler>(id_1, id_2, ThresholdCmd::UseDefault);
    std::array<std::uint8_t, 10> command_buffer{config::SET_LOG_LEVEL,
                                                0x43,
                                                0x4f,
                                                0x52,
                                                0x45,
                                                0x41,
                                                0x50,
                                                0x50,
                                                0x30,
                                                static_cast<uint8_t>(ThresholdCmd::UseDefault)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogLevelHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetLogLevel_OK_ExplicitLevel)
{
    score::platform::dltid_t id_1("CORE");
    score::platform::dltid_t id_2("APP0");
    auto correct_handler = std::make_unique<SetLogLevelHandler>(id_1, id_2, score::mw::log::LogLevel::kVerbose);
    std::array<std::uint8_t, 10> command_buffer{
        config::SET_LOG_LEVEL, 0x43, 0x4f, 0x52, 0x45, 0x41, 0x50, 0x50, 0x30, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogLevelHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetMessagingFilteringState_OK)
{
    auto correct_handler = std::make_unique<SetMessagingFilteringStateHandler>(true);
    std::array<std::uint8_t, 2> command_buffer{config::SET_MESSAGING_FILTERING_STATE, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetMessagingFilteringStateHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetDefaultLogLevel_OK)
{
    auto correct_handler = std::make_unique<SetDefaultLogLevelHandler>(score::mw::log::LogLevel::kFatal);
    std::array<std::uint8_t, 2> command_buffer{config::SET_DEFAULT_LOG_LEVEL, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetDefaultLogLevelHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetLogChannelAssignment_OK)
{
    score::platform::dltid_t id_1("APP0");
    score::platform::dltid_t id_2("CTX0");
    score::platform::dltid_t id_3("CORE");
    auto correct_handler = std::make_unique<SetLogChannelAssignmentHandler>(id_1, id_2, id_3, AssignmentAction::Add);
    std::array<std::uint8_t, 14> command_buffer{
        config::SET_LOG_CHANNEL_ASSIGNMENT, 0x41, 0x50, 0x50, 0x30, 0x43, 0x54, 0x58, 0x30, 0x43, 0x4f, 0x52, 0x45, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogChannelAssignmentHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetDltOutputEnable_OK)
{
    auto correct_handler = std::make_unique<SetDltOutputEnableHandler>(true);
    std::array<std::uint8_t, 2> command_buffer{config::SET_DLT_OUTPUT_ENABLE, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetDltOutputEnableHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

/*-----------------------------------------------------------------
 * 4.  NEGATIVE / ERROR-PATH
 *----------------------------------------------------------------*/
TEST_F(DiagnosticJobParserTest, EmptyCommandWillReturnNullPtr)
{
    auto handler = uut.parse("");
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, UnknownCommand_ReturnsNull)
{
    auto handler = uut.parse("\x7F");
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Threshold_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::SET_LOG_CHANNEL_THRESHOLD)};
    auto handler = uut.parse(command_buffer);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Threshold_InvalidLevel_ReturnsNull)
{
    std::array<std::uint8_t, 7> command_buffer{config::SET_LOG_CHANNEL_THRESHOLD, 1, 2, 3, 4, 0xFF, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, LogLevel_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::SET_LOG_LEVEL)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, LogLevel_InvalidThresholdByte_ReturnsNull)
{
    std::array<std::uint8_t, 10> command_buffer{config::SET_LOG_LEVEL, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, MessagingFiltering_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::SET_MESSAGING_FILTERING_STATE)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, DefaultLogLevel_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::SET_DEFAULT_LOG_LEVEL)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, DefaultLogLevel_InvalidLevel_ReturnsNull)
{
    std::array<std::uint8_t, 2> command_buffer{config::SET_DEFAULT_LOG_LEVEL, 7};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Assignment_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::SET_LOG_CHANNEL_ASSIGNMENT)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Assignment_InvalidAction_ReturnsNull)
{
    std::array<std::uint8_t, 14> command_buffer{
        config::SET_LOG_CHANNEL_ASSIGNMENT, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, OutputEnable_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::SET_DLT_OUTPUT_ENABLE)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, OutputEnable_InvalidFlag_ReturnsNull1)
{
    std::array<std::uint8_t, 2> command_buffer{config::SET_DLT_OUTPUT_ENABLE, 2};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.parse(command);
    ASSERT_EQ(nullptr, handler);
}

}  // namespace test
