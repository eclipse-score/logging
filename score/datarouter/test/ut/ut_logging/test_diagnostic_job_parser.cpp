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
    auto handler = uut.Parse(std::string{char(config::kReadLogChannelNames)});
    EXPECT_NE(ConvertHandlerTypeTo<ReadLogChannelNamesHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, ResetToDefault_OK)
{
    auto correct_handler = std::make_unique<ResetToDefaultHandler>();
    auto handler = uut.Parse(std::string{char(config::kResetToDefault)});
    EXPECT_NE(ConvertHandlerTypeTo<ResetToDefaultHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, StoreDltConfig_OK)
{
    auto correct_handler = std::make_unique<StoreDltConfigHandler>();
    auto handler = uut.Parse(std::string{char(config::kStoreDltConfig)});
    EXPECT_NE(ConvertHandlerTypeTo<StoreDltConfigHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, SetTraceState_OK)
{
    auto correct_handler = std::make_unique<SetTraceStateHandler>();
    auto handler = uut.Parse(std::string{char(config::kSetTraceState)});
    EXPECT_NE(ConvertHandlerTypeTo<SetTraceStateHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, SetDefaultTraceState_OK)
{
    auto correct_handler = std::make_unique<SetDefaultTraceStateHandler>();
    auto handler = uut.Parse(std::string{char(config::kSetDefaultTraceState)});
    EXPECT_NE(ConvertHandlerTypeTo<SetDefaultTraceStateHandler>(handler), nullptr);
}

TEST_F(DiagnosticJobParserTest, SetLogChannelThreshold_OK)
{
    score::platform::DltidT id("CORE");
    auto correct_handler = std::make_unique<SetLogChannelThresholdHandler>(id, score::mw::log::LogLevel::kDebug);
    std::array<std::uint8_t, 7> command_buffer{config::kSetLogChannelThreshold, 0x43, 0x4f, 0x52, 0x45, 5, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogChannelThresholdHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetLogLevel_OK_UseDefault)
{
    score::platform::DltidT id_1("CORE");
    score::platform::DltidT id_2("APP0");
    auto correct_handler = std::make_unique<SetLogLevelHandler>(id_1, id_2, ThresholdCmd::kUseDefault);
    std::array<std::uint8_t, 10> command_buffer{config::kSetLogLevel,
                                                0x43,
                                                0x4f,
                                                0x52,
                                                0x45,
                                                0x41,
                                                0x50,
                                                0x50,
                                                0x30,
                                                static_cast<uint8_t>(ThresholdCmd::kUseDefault)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogLevelHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetLogLevel_OK_ExplicitLevel)
{
    score::platform::DltidT id_1("CORE");
    score::platform::DltidT id_2("APP0");
    auto correct_handler = std::make_unique<SetLogLevelHandler>(id_1, id_2, score::mw::log::LogLevel::kVerbose);
    std::array<std::uint8_t, 10> command_buffer{
        config::kSetLogLevel, 0x43, 0x4f, 0x52, 0x45, 0x41, 0x50, 0x50, 0x30, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogLevelHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetMessagingFilteringState_OK)
{
    auto correct_handler = std::make_unique<SetMessagingFilteringStateHandler>(true);
    std::array<std::uint8_t, 2> command_buffer{config::kSetMessagingFilteringState, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetMessagingFilteringStateHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetDefaultLogLevel_OK)
{
    auto correct_handler = std::make_unique<SetDefaultLogLevelHandler>(score::mw::log::LogLevel::kFatal);
    std::array<std::uint8_t, 2> command_buffer{config::kSetDefaultLogLevel, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetDefaultLogLevelHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetLogChannelAssignment_OK)
{
    score::platform::DltidT id_1("APP0");
    score::platform::DltidT id_2("CTX0");
    score::platform::DltidT id_3("CORE");
    auto correct_handler = std::make_unique<SetLogChannelAssignmentHandler>(id_1, id_2, id_3, AssignmentAction::kAdd);
    std::array<std::uint8_t, 14> command_buffer{
        config::kSetLogChannelAssignment, 0x41, 0x50, 0x50, 0x30, 0x43, 0x54, 0x58, 0x30, 0x43, 0x4f, 0x52, 0x45, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetLogChannelAssignmentHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

TEST_F(DiagnosticJobParserTest, SetDltOutputEnable_OK)
{
    auto correct_handler = std::make_unique<SetDltOutputEnableHandler>(true);
    std::array<std::uint8_t, 2> command_buffer{config::kSetDltOutputEnable, 1};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    const auto* actual = ConvertHandlerTypeTo<SetDltOutputEnableHandler>(handler);
    EXPECT_NE(actual, nullptr);
    EXPECT_EQ(*actual, *correct_handler);
}

/*-----------------------------------------------------------------
 * 4.  NEGATIVE / ERROR-PATH
 *----------------------------------------------------------------*/
TEST_F(DiagnosticJobParserTest, EmptyCommandWillReturnNullPtr)
{
    auto handler = uut.Parse("");
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, UnknownCommand_ReturnsNull)
{
    auto handler = uut.Parse("\x7F");
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Threshold_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::kSetLogChannelThreshold)};
    auto handler = uut.Parse(command_buffer);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Threshold_InvalidLevel_ReturnsNull)
{
    std::array<std::uint8_t, 7> command_buffer{config::kSetLogChannelThreshold, 1, 2, 3, 4, 0xFF, 6};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, LogLevel_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::kSetLogLevel)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, LogLevel_InvalidThresholdByte_ReturnsNull)
{
    std::array<std::uint8_t, 10> command_buffer{config::kSetLogLevel, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, MessagingFiltering_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::kSetMessagingFilteringState)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, DefaultLogLevel_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::kSetDefaultLogLevel)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, DefaultLogLevel_InvalidLevel_ReturnsNull)
{
    std::array<std::uint8_t, 2> command_buffer{config::kSetDefaultLogLevel, 7};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Assignment_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::kSetLogChannelAssignment)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, Assignment_InvalidAction_ReturnsNull)
{
    std::array<std::uint8_t, 14> command_buffer{
        config::kSetLogChannelAssignment, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, OutputEnable_WrongSize_ReturnsNull)
{
    std::string command_buffer{char(config::kSetDltOutputEnable)};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

TEST_F(DiagnosticJobParserTest, OutputEnable_InvalidFlag_ReturnsNull1)
{
    std::array<std::uint8_t, 2> command_buffer{config::kSetDltOutputEnable, 2};
    const std::string command{command_buffer.begin(), command_buffer.end()};
    auto handler = uut.Parse(command);
    ASSERT_EQ(nullptr, handler);
}

}  // namespace test
