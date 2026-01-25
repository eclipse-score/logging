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
std::string MakeTypeParams(dltid_t ecu_id, dltid_t app_id)
{
    return std::string(4, char(0)) + std::string(ecu_id.data(), 4) + std::string(app_id.data(), 4) +
           ::score::common::visitor::logger_type_string<T>();
}

template <typename T>
std::string MakeWrongTypeParams(dltid_t ecu_id, dltid_t app_id)
{
    // Without the first four zeros.
    return std::string(ecu_id.data(), 4) + std::string(app_id.data(), 4) +
           ::score::common::visitor::logger_type_string<T>();
}

template <typename S, typename T>
std::string MakeMessage(S type_index, const T& t)
{
    static constexpr MsgsizeT kMaxMessageSize = 65500;
    std::array<char, kMaxMessageSize> buffer;
    using LoggingSerializer = ::score::common::visitor::logging_serializer;
    const auto index_size = LoggingSerializer::serialize(type_index, buffer.data(), buffer.size());
    const auto size =
        index_size + LoggingSerializer::serialize(t, buffer.data() + index_size, buffer.size() - index_size);
    return std::string{buffer.data(), size};
}

class AnyHandlerMock : public LogParser::AnyHandler
{
  public:
    MOCK_METHOD(void, handle, (const TypeInfo&, timestamp_t, const char*, bufsize_t), (override final));
};

class TypeHandlerMock : public LogParser::TypeHandler
{
  public:
    MOCK_METHOD(void, handle, (timestamp_t, const char*, bufsize_t), (override final));
};

TEST(LogParserTest, SingleMessageHandler)
{
    testing::StrictMock<AnyHandlerMock> any_handler;
    EXPECT_CALL(any_handler, handle(_, _, _, _)).Times(1);
    testing::StrictMock<TypeHandlerMock> type_handler_yes;
    EXPECT_CALL(type_handler_yes, handle(_, _, _)).Times(1);
    testing::StrictMock<TypeHandlerMock> type_handler_no;
    EXPECT_CALL(type_handler_no, handle(_, _, _)).Times(0);

    const timestamp_t time_now = timestamp_t::clock::now();
    const std::string type_params = MakeTypeParams<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t kTestMessageIndex = 1234;
    const std::string message = MakeMessage(kTestMessageIndex, TestMessage{2345});
    LogParser parser(CreateTestNvConfig());
    parser.add_global_handler(any_handler);
    parser.add_type_handler("test::TestMessage", type_handler_yes);
    parser.add_type_handler("test::notTestMessage", type_handler_no);

    parser.add_incoming_type(kTestMessageIndex, type_params);

    parser.parse(time_now, message.data(), static_cast<bufsize_t>(message.size()));
}

TEST(LogParserTest, FilterForwarderWithSingleForwarder)
{
    using namespace std::chrono_literals;
    const std::string type_params = MakeTypeParams<TestMessage>(dltid_t{"ECU4"}, dltid_t{"APP0"});

    LogParser parser(CreateTestNvConfig());
    const auto factory = [](const std::string& type_name, const DataFilter& filter) -> LogParser::FilterFunction {
        if (type_name == "test::TestMessage" && filter.filterType == "test::TestFilter")
        {
            TestFilter test_filter;
            using S = ::score::common::visitor::logging_serializer;
            if (S::deserialize(filter.filterData.data(), filter.filterData.size(), test_filter))
            {
                return [test_filter](const char* const data, const bufsize_t size) {
                    TestMessage message;
                    if (S::deserialize(data, size, message))
                    {
                        return test_filter.test_field == message.test_field;
                    }
                    else
                    {
                        return false;
                    }
                };
            }
        }
        return {};
    };
    parser.set_filter_factory(factory);

    constexpr bufsize_t kTestMessageIndex = 1234;
    parser.add_incoming_type(kTestMessageIndex, type_params);

    const std::string message1 = MakeMessage(kTestMessageIndex, TestMessage{1});
    parser.parse(timestamp_t{1s}, message1.data(), static_cast<bufsize_t>(message1.size()));
    const std::string message2 = MakeMessage(kTestMessageIndex, TestMessage{2});
    parser.parse(timestamp_t{2s}, message2.data(), static_cast<bufsize_t>(message2.size()));
    const std::string message3 = MakeMessage(kTestMessageIndex, TestMessage{3});
    parser.parse(timestamp_t{3s}, message3.data(), static_cast<bufsize_t>(message3.size()));
}

// Test the else case in the below condition in 'remove_type_handler' and 'remove_handler' methods.
// The conditions are:
// if (ith != ith_range.second)
// if (it != handlers_.end())
TEST(LogParserTest, TestRemoveTypeHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<TypeHandlerMock> type_handler_yes;
    parser.add_type_handler("test::TestMessage", type_handler_yes);
    parser.add_type_handler("test::TestMessage", type_handler_yes);

    testing::StrictMock<TypeHandlerMock> type_handler_no;
    EXPECT_CALL(type_handler_no, handle(_, _, _)).Times(0);

    parser.add_type_handler("test::notTestMessage", type_handler_no);

    const std::string type_params = MakeTypeParams<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t kTestMessageIndex = 1234;

    // Add the type twice.
    parser.add_incoming_type(kTestMessageIndex, type_params);
    parser.add_incoming_type(kTestMessageIndex, type_params);
    parser.remove_type_handler("test::TestMessage", type_handler_yes);
    // Remove non existent type handler.
    parser.remove_type_handler("test::TestMessage", type_handler_yes);
}

// Test the True case for the below condition for 'add_incoming_type' method.
// The condition is:
// if (params.size() <= 12 + sizeof(uint32_t) || params[0] != 0 || params[1] != 0 || params[2] != 0 || params[3] != 0)
// There is no expectation or assertion we can set to check this condition.
TEST(LogParserTest, TestWrongTypeParameter)
{
    LogParser parser(CreateTestNvConfig());
    const std::string type_params = MakeWrongTypeParams<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t kTestMessageIndex = 1234;
    parser.add_incoming_type(kTestMessageIndex, type_params);
}

// The purpose of the test is to cover the else case for the below condition for 'add_global_handler' method.
// The condition is:
// if (is_glb_hndl_registered(handler) == false)
TEST(LogParserTest, TestRegisterGlobalHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<AnyHandlerMock> any_handler;
    // Register the same global handler twice.
    EXPECT_FALSE(parser.is_glb_hndl_registered(any_handler));
    parser.add_global_handler(any_handler);
    EXPECT_TRUE(parser.is_glb_hndl_registered(any_handler));
    // To reach the else case in the condition there.
    parser.add_global_handler(any_handler);
}

// The purpose of the test is to cover the else case for the below condition for 'remove_global_handler' method.
// The condition is:
// if (it != global_handlers.end())
TEST(LogParserTest, TestRemovingGlobalHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<AnyHandlerMock> any_handler;
    // Check non-registered handler.
    EXPECT_FALSE(parser.is_glb_hndl_registered(any_handler));
    // Register new handler.
    parser.add_global_handler(any_handler);
    // Check registered handler.
    EXPECT_TRUE(parser.is_glb_hndl_registered(any_handler));
    // Remove the handler.
    parser.remove_global_handler(any_handler);
    // Handler no more exist.
    EXPECT_FALSE(parser.is_glb_hndl_registered(any_handler));
    // Try remove non registered handler (To reach the else case in the condition there).
    parser.remove_global_handler(any_handler);
}

// Test the if condition in the 'add_type_handler' method.
// The condition is:
// if (is_type_hndl_registered(typeName, handler))
TEST(LogParserTest, TestAlreadyRegisteredTypeHandler)
{
    testing::StrictMock<TypeHandlerMock> type_handler_yes;

    LogParser parser(CreateTestNvConfig());
    parser.add_type_handler("test::TestMessage", type_handler_yes);
    parser.add_type_handler("test::TestMessage", type_handler_yes);

    EXPECT_TRUE(parser.is_type_hndl_registered("test::TestMessage", type_handler_yes));
}

TEST(LogParserTest, TestRegisteringNewTypeHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<TypeHandlerMock> type_handler_yes;

    EXPECT_FALSE(parser.is_type_hndl_registered("test::TestMessage", type_handler_yes));

    const std::string type_params = MakeTypeParams<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t kTestMessageIndex = 1234;
    parser.add_incoming_type(kTestMessageIndex, type_params);
    parser.add_type_handler("test::TestMessage", type_handler_yes);

    EXPECT_TRUE(parser.is_type_hndl_registered("test::TestMessage", type_handler_yes));
}

// The purpose of this test is to enhance the line coverage for 'reset_internal_mapping' method.
TEST(LogParserTest, TestResetInternalMapping)
{
    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    EXPECT_NO_FATAL_FAILURE(parser.reset_internal_mapping());
}

struct SmallTestMessage
{
    uint8_t test_field;
};
STRUCT_VISITABLE(SmallTestMessage, test_field)

// The purpose of this test is to enhance the line coverage for
// parse(timestamp_t timestamp, const char* data, bufsize_t size) method.
TEST(LogParserTest, WeCanNotParseIfTheSizeOfTheSerializedMessageSmallerThanTheExpectedBufferSizeUint32)
{
    const timestamp_t time_now = timestamp_t::clock::now();
    constexpr uint8_t kSmallTestMessageIndex = 3;
    const std::string message = MakeMessage(kSmallTestMessageIndex, SmallTestMessage{7});

    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    EXPECT_NO_FATAL_FAILURE(parser.parse(time_now, message.data(), static_cast<std::uint16_t>(message.size())));
    EXPECT_NO_FATAL_FAILURE(parser.reset_internal_mapping());
    EXPECT_NO_FATAL_FAILURE(parser.parse(time_now, message.data(), static_cast<std::uint16_t>(message.size())));
}

// The purpose of this test is to enhance the line coverage for 'parse' with covering the below condition.
// The condition is:
// if (iParser == index_parser_map.end())
TEST(LogParserTest, WeCanNotParseIfTheIndexIsNotWithinTheIndexParserMap)
{
    const timestamp_t time_now = timestamp_t::clock::now();
    constexpr bufsize_t kTestMessageIndex = 1235;
    const std::string message = MakeMessage(kTestMessageIndex, TestMessage{1234});

    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    EXPECT_NO_FATAL_FAILURE(parser.parse(time_now, message.data(), static_cast<bufsize_t>(message.size())));
    // Since we didn't fill any values to 'index_parser_map' map, it will be empty which leads to immediate returning.
}

TEST(LogParserTest, WeCanNotParseASharedMemoryRecordIfTheTypeIdentifierIsNotWithinTheIndexParserMap)
{
    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    score::mw::log::detail::SharedMemoryRecord record;
    EXPECT_NO_FATAL_FAILURE(parser.Parse(record));
    // Since we didn't fill any values to 'index_parser_map' map, it will be empty which leads to immediate returning.
}

}  // namespace test
