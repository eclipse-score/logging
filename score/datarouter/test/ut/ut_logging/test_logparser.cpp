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

#include "logparser/logparser.h"

#include "score/mw/log/configuration/invconfig_mock.h"

#include "static_reflection_with_serialization/serialization/for_logging.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

using MsgsizeT = std::uint16_t;

// Helper function to get the mock NvConfig for LogParser tests
static score::mw::log::INvConfig& CreateTestNvConfig()
{
    // Global mock NvConfig for LogParser tests
    static score::mw::log::INvConfigMock g_nvconfig_mock;
    return g_nvconfig_mock;
}

namespace test
{

using namespace score::platform;
using namespace score::platform::internal;

struct TestMessage
{
    int32_t test_field;
};

struct TestFilter
{
    int32_t test_field;
};

STRUCT_VISITABLE(TestMessage, test_field)
STRUCT_VISITABLE(TestFilter, test_field)

template <typename T>
std::string MakeTypeParams(DltidT ecu_id, DltidT app_id)
{
    return std::string(4, char(0)) + std::string(ecu_id) + std::string(app_id) +
           ::score::common::visitor::logger_type_string<T>();
}

template <typename T>
std::string MakeWrongTypeParams(DltidT ecu_id, DltidT app_id)
{
    // Without the first four zeros.
    return std::string(ecu_id) + std::string(app_id) + ::score::common::visitor::logger_type_string<T>();
}

template <typename S, typename T>
std::string MakeMessage(S type_index, const T& t)
{
    static constexpr MsgsizeT kMaxMessageSize = 65500;
    std::array<char, kMaxMessageSize> buffer{};
    using LoggingSerializer = ::score::common::visitor::logging_serializer;
    const auto index_size = LoggingSerializer::serialize(type_index, buffer.data(), buffer.size());
    const auto size =
        index_size + LoggingSerializer::serialize(t, buffer.data() + index_size, buffer.size() - index_size);
    return std::string{buffer.data(), size};
}

class AnyHandlerMock : public LogParser::AnyHandler
{
  public:
    MOCK_METHOD(void, Handle, (const TypeInfo&, TimestampT, const char*, BufsizeT), (override final));
};

class TypeHandlerMock : public LogParser::TypeHandler
{
  public:
    MOCK_METHOD(void, Handle, (TimestampT, const char*, BufsizeT), (override final));
};

TEST(LogParserTest, SingleMessageHandler)
{
    testing::StrictMock<AnyHandlerMock> any_handler;
    EXPECT_CALL(any_handler, Handle(_, _, _, _)).Times(1);
    testing::StrictMock<TypeHandlerMock> type_handler_yes;
    EXPECT_CALL(type_handler_yes, Handle(_, _, _)).Times(1);
    testing::StrictMock<TypeHandlerMock> type_handler_no;
    EXPECT_CALL(type_handler_no, Handle(_, _, _)).Times(0);

    const TimestampT time_now = TimestampT::clock::now();
    const std::string type_params = MakeTypeParams<TestMessage>(DltidT{"ECU0"}, DltidT{"APP0"});
    constexpr BufsizeT kTestMessageIndex = 1234;
    const std::string message = MakeMessage(kTestMessageIndex, TestMessage{2345});
    LogParser parser(CreateTestNvConfig(),
                     {&any_handler},
                     {{"test::TestMessage", &type_handler_yes}, {"test::notTestMessage", &type_handler_no}});

    parser.AddIncomingType(kTestMessageIndex, type_params);

    parser.Parse(time_now, message.data(), static_cast<BufsizeT>(message.size()));
}

TEST(LogParserTest, FilterForwarderWithSingleForwarder)
{
    using namespace std::chrono_literals;
    const std::string type_params = MakeTypeParams<TestMessage>(DltidT{"ECU4"}, DltidT{"APP0"});

    LogParser parser(CreateTestNvConfig());

    constexpr BufsizeT kTestMessageIndex = 1234;
    parser.AddIncomingType(kTestMessageIndex, type_params);

    const std::string message1 = MakeMessage(kTestMessageIndex, TestMessage{1});
    parser.Parse(TimestampT{1s}, message1.data(), static_cast<BufsizeT>(message1.size()));
    const std::string message2 = MakeMessage(kTestMessageIndex, TestMessage{2});
    parser.Parse(TimestampT{2s}, message2.data(), static_cast<BufsizeT>(message2.size()));
    const std::string message3 = MakeMessage(kTestMessageIndex, TestMessage{3});
    parser.Parse(TimestampT{3s}, message3.data(), static_cast<BufsizeT>(message3.size()));
}

// Test the True case for the below condition for 'add_incoming_type' method.
// The condition is:
// if (params.size() <= 12 + sizeof(uint32_t) || params[0] != 0 || params[1] != 0 || params[2] != 0 || params[3] != 0)
// There is no expectation or assertion we can set to check this condition.
TEST(LogParserTest, TestWrongTypeParameter)
{
    LogParser parser(CreateTestNvConfig());
    const std::string type_params = MakeWrongTypeParams<TestMessage>(DltidT{"ECU0"}, DltidT{"APP0"});
    constexpr BufsizeT kTestMessageIndex = 1234;
    parser.AddIncomingType(kTestMessageIndex, type_params);
}

// Test that global handlers passed to constructor are registered.
TEST(LogParserTest, TestRegisterGlobalHandler)
{
    testing::StrictMock<AnyHandlerMock> any_handler;
    LogParser parser(CreateTestNvConfig(), {&any_handler});
    EXPECT_TRUE(parser.IsGlbHndlRegistered(any_handler));
}

// Test that type handlers passed to constructor are registered.
TEST(LogParserTest, TestAlreadyRegisteredTypeHandler)
{
    testing::StrictMock<TypeHandlerMock> type_handler_yes;

    LogParser parser(CreateTestNvConfig(), {}, {{"test::TestMessage", &type_handler_yes}});

    EXPECT_TRUE(parser.IsTypeHndlRegistered("test::TestMessage", type_handler_yes));
}

TEST(LogParserTest, TestRegisteringNewTypeHandler)
{
    testing::StrictMock<TypeHandlerMock> type_handler_yes;
    LogParser parser(CreateTestNvConfig(), {}, {{"test::TestMessage", &type_handler_yes}});

    EXPECT_TRUE(parser.IsTypeHndlRegistered("test::TestMessage", type_handler_yes));

    const std::string type_params = MakeTypeParams<TestMessage>(DltidT{"ECU0"}, DltidT{"APP0"});
    constexpr BufsizeT kTestMessageIndex = 1234;
    parser.AddIncomingType(kTestMessageIndex, type_params);

    EXPECT_TRUE(parser.IsTypeHndlRegistered("test::TestMessage", type_handler_yes));
}

struct SmallTestMessage
{
    uint8_t test_field;
};
STRUCT_VISITABLE(SmallTestMessage, test_field)

// The purpose of this test is to enhance the line coverage for
// parse(TimestampT timestamp, const char* data, BufsizeT size) method.
TEST(LogParserTest, WeCanNotParseIfTheSizeOfTheSerializedMessageSmallerThanTheExpectedBufferSizeUint32)
{
    const TimestampT time_now = TimestampT::clock::now();
    constexpr uint8_t kSmallTestMessageIndex = 3;
    const std::string message = MakeMessage(kSmallTestMessageIndex, SmallTestMessage{7});

    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    EXPECT_NO_FATAL_FAILURE(parser.Parse(time_now, message.data(), static_cast<std::uint16_t>(message.size())));
}

// The purpose of this test is to enhance the line coverage for 'parse' with covering the below condition.
// The condition is:
// if (iParser == index_parser_map.end())
TEST(LogParserTest, WeCanNotParseIfTheIndexIsNotWithinTheIndexParserMap)
{
    const TimestampT time_now = TimestampT::clock::now();
    constexpr BufsizeT kTestMessageIndex = 1235;
    const std::string message = MakeMessage(kTestMessageIndex, TestMessage{1234});

    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    EXPECT_NO_FATAL_FAILURE(parser.Parse(time_now, message.data(), static_cast<BufsizeT>(message.size())));
    // Since we didn't fill any values to 'index_parser_map' map, it will be empty which leads to immediate returning.
}

TEST(LogParserTest, WeCanNotParseASharedMemoryRecordIfTheTypeIdentifierIsNotWithinTheIndexParserMap)
{
    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    score::mw::log::detail::SharedMemoryRecord record;
    EXPECT_NO_FATAL_FAILURE(parser.ParseSharedMemoryRecord(record));
    // Since we didn't fill any values to 'index_parser_map' map, it will be empty which leads to immediate returning.
}

}  // namespace test
