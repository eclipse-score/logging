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

using msgsize_t = std::uint16_t;

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
    int32_t testField;
};

struct TestFilter
{
    int32_t testField;
};

STRUCT_VISITABLE(TestMessage, testField)
STRUCT_VISITABLE(TestFilter, testField)

template <typename T>
std::string make_type_params(dltid_t ecuId, dltid_t appId)
{
    return std::string(4, char(0)) + std::string(ecuId.data(), 4) + std::string(appId.data(), 4) +
           ::score::common::visitor::logger_type_string<T>();
}

template <typename T>
std::string make_wrong_type_params(dltid_t ecuId, dltid_t appId)
{
    // Without the first four zeros.
    return std::string(ecuId.data(), 4) + std::string(appId.data(), 4) +
           ::score::common::visitor::logger_type_string<T>();
}

template <typename S, typename T>
std::string make_message(S typeIndex, const T& t)
{
    static constexpr msgsize_t MaxMessageSize = 65500;
    std::array<char, MaxMessageSize> buffer;
    using s = ::score::common::visitor::logging_serializer;
    const auto indexSize = s::serialize(typeIndex, buffer.data(), buffer.size());
    const auto size = indexSize + s::serialize(t, buffer.data() + indexSize, buffer.size() - indexSize);
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
    testing::StrictMock<AnyHandlerMock> anyHandler;
    EXPECT_CALL(anyHandler, handle(_, _, _, _)).Times(1);
    testing::StrictMock<TypeHandlerMock> typeHandlerYes;
    EXPECT_CALL(typeHandlerYes, handle(_, _, _)).Times(1);
    testing::StrictMock<TypeHandlerMock> typeHandlerNo;
    EXPECT_CALL(typeHandlerNo, handle(_, _, _)).Times(0);

    const timestamp_t timeNow = timestamp_t::clock::now();
    const std::string typeParams = make_type_params<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t testMessageIndex = 1234;
    const std::string message = make_message(testMessageIndex, TestMessage{2345});
    LogParser parser(CreateTestNvConfig());
    parser.add_global_handler(anyHandler);
    parser.add_type_handler("test::TestMessage", typeHandlerYes);
    parser.add_type_handler("test::notTestMessage", typeHandlerNo);

    parser.add_incoming_type(testMessageIndex, typeParams);

    parser.parse(timeNow, message.data(), static_cast<bufsize_t>(message.size()));
}

TEST(LogParserTest, FilterForwarderWithSingleForwarder)
{
    using namespace std::chrono_literals;
    const std::string typeParams = make_type_params<TestMessage>(dltid_t{"ECU4"}, dltid_t{"APP0"});

    LogParser parser(CreateTestNvConfig());
    const auto factory = [](const std::string& typeName, const DataFilter& filter) -> LogParser::FilterFunction {
        if (typeName == "test::TestMessage" && filter.filterType == "test::TestFilter")
        {
            TestFilter testFilter;
            using s = ::score::common::visitor::logging_serializer;
            if (s::deserialize(filter.filterData.data(), filter.filterData.size(), testFilter))
            {
                return [testFilter](const char* const data, const bufsize_t size) {
                    TestMessage message;
                    if (s::deserialize(data, size, message))
                    {
                        return testFilter.testField == message.testField;
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

    constexpr bufsize_t testMessageIndex = 1234;
    parser.add_incoming_type(testMessageIndex, typeParams);

    const std::string message1 = make_message(testMessageIndex, TestMessage{1});
    parser.parse(timestamp_t{1s}, message1.data(), static_cast<bufsize_t>(message1.size()));
    const std::string message2 = make_message(testMessageIndex, TestMessage{2});
    parser.parse(timestamp_t{2s}, message2.data(), static_cast<bufsize_t>(message2.size()));
    const std::string message3 = make_message(testMessageIndex, TestMessage{3});
    parser.parse(timestamp_t{3s}, message3.data(), static_cast<bufsize_t>(message3.size()));
}

// Test the else case in the below condition in 'remove_type_handler' and 'remove_handler' methods.
// The conditions are:
// if (ith != ith_range.second)
// if (it != handlers_.end())
TEST(LogParserTest, TestRemoveTypeHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<TypeHandlerMock> typeHandlerYes;
    parser.add_type_handler("test::TestMessage", typeHandlerYes);
    parser.add_type_handler("test::TestMessage", typeHandlerYes);

    testing::StrictMock<TypeHandlerMock> typeHandlerNo;
    EXPECT_CALL(typeHandlerNo, handle(_, _, _)).Times(0);

    parser.add_type_handler("test::notTestMessage", typeHandlerNo);

    const std::string typeParams = make_type_params<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t testMessageIndex = 1234;

    // Add the type twice.
    parser.add_incoming_type(testMessageIndex, typeParams);
    parser.add_incoming_type(testMessageIndex, typeParams);
    parser.remove_type_handler("test::TestMessage", typeHandlerYes);
    // Remove non existent type handler.
    parser.remove_type_handler("test::TestMessage", typeHandlerYes);
}

// Test the True case for the below condition for 'add_incoming_type' method.
// The condition is:
// if (params.size() <= 12 + sizeof(uint32_t) || params[0] != 0 || params[1] != 0 || params[2] != 0 || params[3] != 0)
// There is no expectation or assertion we can set to check this condition.
TEST(LogParserTest, TestWrongTypeParameter)
{
    LogParser parser(CreateTestNvConfig());
    const std::string typeParams = make_wrong_type_params<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t testMessageIndex = 1234;
    parser.add_incoming_type(testMessageIndex, typeParams);
}

// The purpose of the test is to cover the else case for the below condition for 'add_global_handler' method.
// The condition is:
// if (is_glb_hndl_registered(handler) == false)
TEST(LogParserTest, TestRegisterGlobalHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<AnyHandlerMock> anyHandler;
    // Register the same global handler twice.
    EXPECT_FALSE(parser.is_glb_hndl_registered(anyHandler));
    parser.add_global_handler(anyHandler);
    EXPECT_TRUE(parser.is_glb_hndl_registered(anyHandler));
    // To reach the else case in the condition there.
    parser.add_global_handler(anyHandler);
}

// The purpose of the test is to cover the else case for the below condition for 'remove_global_handler' method.
// The condition is:
// if (it != global_handlers.end())
TEST(LogParserTest, TestRemovingGlobalHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<AnyHandlerMock> anyHandler;
    // Check non-registered handler.
    EXPECT_FALSE(parser.is_glb_hndl_registered(anyHandler));
    // Register new handler.
    parser.add_global_handler(anyHandler);
    // Check registered handler.
    EXPECT_TRUE(parser.is_glb_hndl_registered(anyHandler));
    // Remove the handler.
    parser.remove_global_handler(anyHandler);
    // Handler no more exist.
    EXPECT_FALSE(parser.is_glb_hndl_registered(anyHandler));
    // Try remove non registered handler (To reach the else case in the condition there).
    parser.remove_global_handler(anyHandler);
}

// Test the if condition in the 'add_type_handler' method.
// The condition is:
// if (is_type_hndl_registered(typeName, handler))
TEST(LogParserTest, TestAlreadyRegisteredTypeHandler)
{
    testing::StrictMock<TypeHandlerMock> typeHandlerYes;

    LogParser parser(CreateTestNvConfig());
    parser.add_type_handler("test::TestMessage", typeHandlerYes);
    parser.add_type_handler("test::TestMessage", typeHandlerYes);

    EXPECT_TRUE(parser.is_type_hndl_registered("test::TestMessage", typeHandlerYes));
}

TEST(LogParserTest, TestRegisteringNewTypeHandler)
{
    LogParser parser(CreateTestNvConfig());
    testing::StrictMock<TypeHandlerMock> typeHandlerYes;

    EXPECT_FALSE(parser.is_type_hndl_registered("test::TestMessage", typeHandlerYes));

    const std::string typeParams = make_type_params<TestMessage>(dltid_t{"ECU0"}, dltid_t{"APP0"});
    constexpr bufsize_t testMessageIndex = 1234;
    parser.add_incoming_type(testMessageIndex, typeParams);
    parser.add_type_handler("test::TestMessage", typeHandlerYes);

    EXPECT_TRUE(parser.is_type_hndl_registered("test::TestMessage", typeHandlerYes));
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
    uint8_t testField;
};
STRUCT_VISITABLE(SmallTestMessage, testField)

// The purpose of this test is to enhance the line coverage for
// parse(timestamp_t timestamp, const char* data, bufsize_t size) method.
TEST(LogParserTest, WeCanNotParseIfTheSizeOfTheSerializedMessageSmallerThanTheExpectedBufferSizeUint32)
{
    const timestamp_t timeNow = timestamp_t::clock::now();
    constexpr uint8_t smallTestMessageIndex = 3;
    const std::string message = make_message(smallTestMessageIndex, SmallTestMessage{7});

    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    EXPECT_NO_FATAL_FAILURE(parser.parse(timeNow, message.data(), static_cast<std::uint16_t>(message.size())));
    EXPECT_NO_FATAL_FAILURE(parser.reset_internal_mapping());
    EXPECT_NO_FATAL_FAILURE(parser.parse(timeNow, message.data(), static_cast<std::uint16_t>(message.size())));
}

// The purpose of this test is to enhance the line coverage for 'parse' with covering the below condition.
// The condition is:
// if (iParser == index_parser_map.end())
TEST(LogParserTest, WeCanNotParseIfTheIndexIsNotWithinTheIndexParserMap)
{
    const timestamp_t timeNow = timestamp_t::clock::now();
    constexpr bufsize_t TestMessageIndex = 1235;
    const std::string message = make_message(TestMessageIndex, TestMessage{1234});

    LogParser parser(CreateTestNvConfig());
    // Unfortunately, there other way to set expectation for calling this method.
    // And there is no other methods are using it internally.
    EXPECT_NO_FATAL_FAILURE(parser.parse(timeNow, message.data(), static_cast<bufsize_t>(message.size())));
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
