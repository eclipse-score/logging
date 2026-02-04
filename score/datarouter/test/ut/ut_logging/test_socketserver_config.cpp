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

#include "score/mw/log/detail/log_entry.h"
#include "score/mw/log/log_common.h"

#include "daemon/socketserver_filter_factory.h"

#include "score/mw/log/log_level.h"

#include "score/datarouter/include/daemon/socketserver_config.h"
#include "score/datarouter/include/dlt/logentry_trace.h"
#include "score/datarouter/src/persistency/mock_persistent_dictionary.h"
#include "score/datarouter/src/persistency/stub_persistent_dictionary/stub_persistent_dictionary.h"
#include <rapidjson/stringbuffer.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

namespace score
{
namespace platform
{
namespace datarouter
{
namespace
{

const std::string CONFIG_DATABASE_KEY = "dltConfig";
const std::string CONFIG_OUTPUT_ENABLED_KEY = "dltOutputEnabled";

template <typename T>
std::string type_name()
{
    return ::score::common::visitor::struct_visitable<T>::name();
}

TEST(SocketserverConfigTest, FilterFactoryDefault)
{
    const auto factory = ::score::platform::datarouter::getFilterFactory();
    EXPECT_TRUE(factory);
    EXPECT_FALSE(factory("", ::score::platform::DataFilter{}));
}

TEST(SocketserverConfigTest, FilterFactoryLogEntry)
{
    const auto factory = ::score::platform::datarouter::getFilterFactory();

    using ::score::mw::log::detail::LogEntry;
    using ::score::platform::dltid_t;
    using ::score::platform::internal::LogEntryFilter;
    using s = ::score::common::visitor::logging_serializer;

    constexpr std::size_t kSerializationBufferSize = 128;
    std::array<char, kSerializationBufferSize> buffer;

    const LogEntryFilter filter{
        score::mw::log::detail::LoggingIdentifier{"APP0"}, score::mw::log::detail::LoggingIdentifier{""}, 1U};
    const auto fSize = s::serialize(filter, buffer.data(), buffer.size());
    ::score::platform::DataFilter dataFilter{type_name<LogEntryFilter>(), std::string(buffer.data(), fSize)};

    const auto matcher = factory(type_name<LogEntry>(), dataFilter);
    EXPECT_TRUE(matcher);

    const LogEntry entry1{score::mw::log::detail::LoggingIdentifier{"APP0"},
                          score::mw::log::detail::LoggingIdentifier{"CTX0"},
                          {'1'},
                          {},
                          {},
                          1,
                          {},
                          mw::log::LogLevel::kOff};
    const LogEntry entry2{score::mw::log::detail::LoggingIdentifier{"APP0"},
                          score::mw::log::detail::LoggingIdentifier{"CTX0"},
                          {'2'},
                          {},
                          {},
                          1,
                          {},
                          mw::log::LogLevel::kError};
    const LogEntry entry3{score::mw::log::detail::LoggingIdentifier{"APP1"},
                          score::mw::log::detail::LoggingIdentifier{"CTX0"},
                          {'3'},
                          {},
                          {},
                          1,
                          {},
                          mw::log::LogLevel::kOff};
    const auto tSize1 = s::serialize(entry1, buffer.data(), buffer.size());
    EXPECT_TRUE(matcher(buffer.data(), tSize1));
    const auto tSize2 = s::serialize(entry2, buffer.data(), buffer.size());
    EXPECT_FALSE(matcher(buffer.data(), tSize2));
    const auto tSize3 = s::serialize(entry3, buffer.data(), buffer.size());
    EXPECT_FALSE(matcher(buffer.data(), tSize3));
    // Test deserialization for failing and return false.
    EXPECT_FALSE(matcher(buffer.data(), 0));
}

std::string PrepareLogChannelsPath(const std::string& file_name)
{
    const std::string override_path = "score/datarouter/test/ut/etc/" + file_name;
    return override_path;
}

// readStaticDlt unit tests

TEST(SocketserverConfigTest, ReadCorrectLogChannelsNoErrorsExpected)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels.json").c_str());
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(result.value().channels, SizeIs(3));
    EXPECT_THAT(result.value().channelAssignments, SizeIs(2));
    EXPECT_THAT(result.value().messageThresholds, SizeIs(3));
    EXPECT_TRUE(result.value().filteringEnabled);
}

TEST(SocketserverConfigTest, ReadNonExistingPathErrorExpected)
{
    const auto result = readStaticDlt("");
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, ReadEmptyLogChannelErrorExpected)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-empty.json").c_str());
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, JsonWithoutChannelsErrorExpected)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-without-channels.json").c_str());
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, JsonEmptyChannelsErrorExpected)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-empty-channels.json").c_str());
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, JsonFilteringEnabledExpectConfigFilterTrue)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-filtering-enabled.json").c_str());
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().filteringEnabled);
}

TEST(SocketserverConfigTest, JsonQuotasEnabledExpectConfigFilterTrue)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-quotas.json").c_str());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().throughput.overallMbps, 100);
    EXPECT_FALSE(result.value().quotaEnforcementEnabled);
    EXPECT_THAT(result.value().throughput.applicationsKbps, SizeIs(1));
}

TEST(SocketserverConfigTest, JsonQuotasEnabledActivated)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-quotas-activated.json").c_str());
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().quotaEnforcementEnabled);
}

TEST(SocketserverConfigTest, JsonQuotasEnabledDeactivated)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-quotas-deactivated.json").c_str());
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().quotaEnforcementEnabled);
}

TEST(SocketserverConfigTest, JsonOldFormatErrorExpected)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-old-format.json").c_str());
    EXPECT_FALSE(result.has_value());
}

// readDlt unit tests

TEST(SocketserverConfigTest, PersistentDictionaryEmptyJsonErrorExpected)
{
    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, getString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return("{}"));
    const auto result = readDlt(pd);
    EXPECT_THAT(result.channels, SizeIs(0));
}

TEST(SocketserverConfigTest, PersistentDictionaryCorrectJsonNoErrorsExpected)
{
    const std::string expected_json{
        "{\"channels\":{\"3491\":{\"address\":\"0.0.0.0\",\"channelThreshold\":\"kError\",\"dstAddress\":\"239.255.42."
        "99\",\"dstPort\":3490,\"ecu\":\"TST1\",\"port\":3491},\"3492\":{\"address\":\"0.0.0.0\",\"channelThreshold\":"
        "\"kInfo\",\"dstAddress\":\"239.255.42.99\",\"dstPort\":3490,\"ecu\":\"TST2\",\"port\":3492},\"3493\":{"
        "\"address\":\"0.0.0.0\",\"channelThreshold\":\"kVerbose\",\"dstAddress\":\"239.255.42.99\",\"dstPort\":3490,"
        "\"ecu\":\"TST3\",\"port\":3493}},\"channelAssignments\":{\"DR\":{\"\":[\"3492\"],\"CTX1\":[\"3492\",\"3493\"]}"
        ",\"-NI-\":{\"\":[\"3491\"]}},\"filteringEnabled\":true,\"defaultChannel\":\"3493\",\"defaultThresold\":"
        "\"kVerbose\",\"messageThresholds\":{\"\":{\"vcip\":\"kInfo\"},\"DR\":{\"\":\"kVerbose\",\"CTX1\":\"kVerbose\","
        "\"STAT\":\"kDebug\"},\"-NI-\":{\"\":\"kVerbose\"}}}"};

    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, getString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return(expected_json));
    const auto result = readDlt(pd);
    EXPECT_TRUE(result.filteringEnabled);
    EXPECT_THAT(result.channels, SizeIs(3));
    EXPECT_THAT(result.channelAssignments, SizeIs(2));
    EXPECT_THAT(result.messageThresholds, SizeIs(3));
}

TEST(SocketserverConfigTest, PersistentDictionaryEmptyChannelsErrorExpected)
{
    const std::string expected_json{
        "{\"channels\":{},\"channelAssignments\":{\"DR\":{\"\":[\"3492\"],\"CTX1\":[\"3492\",\"3493\"]},\"-NI-\":{\"\":"
        "[\"3491\"]}},\"filteringEnabled\":true,\"defaultChannel\":\"3493\",\"defaultThresold\":\"kVerbose\","
        "\"messageThresholds\":{\"\":{\"vcip\":\"kInfo\"},\"DR\":{\"\":\"kVerbose\",\"CTX1\":\"kVerbose\",\"STAT\":"
        "\"kDebug\"},\"-NI-\":{\"\":\"kVerbose\"}}}"};

    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, getString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return(expected_json));
    const auto result = readDlt(pd);
    EXPECT_THAT(result.channels, SizeIs(0));
}

TEST(SocketserverConfigTest, PersistentDictionaryNoFilteringEnabledExpectTrueByDefault)
{
    const std::string expected_json{
        "{\"channels\":{\"3491\":{\"address\":\"0.0.0.0\",\"channelThreshold\":\"kError\",\"dstAddress\":\"239.255.42."
        "99\",\"dstPort\":3490,\"ecu\":\"TST1\",\"port\":3491},\"3492\":{\"address\":\"0.0.0.0\",\"channelThreshold\":"
        "\"kInfo\",\"dstAddress\":\"239.255.42.99\",\"dstPort\":3490,\"ecu\":\"TST2\",\"port\":3492},\"3493\":{"
        "\"address\":\"0.0.0.0\",\"channelThreshold\":\"kVerbose\",\"dstAddress\":\"239.255.42.99\",\"dstPort\":3490,"
        "\"ecu\":\"TST3\",\"port\":3493}},\"channelAssignments\":{\"DR\":{\"\":[\"3492\"],\"CTX1\":[\"3492\",\"3493\"]}"
        ",\"-NI-\":{\"\":[\"3491\"]}},\"defaultChannel\":\"3493\",\"defaultThresold\":\"kVerbose\","
        "\"messageThresholds\":{\"\":{\"vcip\":\"kInfo\"},\"DR\":{\"\":\"kVerbose\",\"CTX1\":\"kVerbose\",\"STAT\":"
        "\"kDebug\"},\"-NI-\":{\"\":\"kVerbose\"}}}"};

    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, getString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return(expected_json));
    const auto result = readDlt(pd);
    EXPECT_TRUE(result.filteringEnabled);
}

// writeDltEnabled unit test

TEST(SocketserverConfigTest, WriteDltEnabledCallSetBoolExpected)
{
    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, setBool(StrEq("dltOutputEnabled"), true)).Times(1);
    writeDltEnabled(true, pd);
}

// readDltEnabled unit test

TEST(SocketserverConfigTest, ReadDltEnabledTrueResultExpected)
{
    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, getBool(StrEq("dltOutputEnabled"), true)).WillOnce(Return(true));
    const auto result = readDltEnabled(pd);
    EXPECT_TRUE(result);
}

// writeDlt unit tests

TEST(SocketserverConfigTest, WriteDltFilledPersistentConfigNoErrorExpected)
{
    const std::string expected_json{
        "{\"channels\":{\"3491\":{\"channelThreshold\":\"kVerbose\"}},\"channelAssignments\":{\"000\":{\"111\":["
        "\"2222\"]}},\"filteringEnabled\":true,\"defaultThresold\":\"kVerbose\",\"messageThresholds\":{\"000\":{"
        "\"111\":\"kVerbose\"}}}"};
    StrictMock<MockPersistentDictionary> pd;
    score::logging::dltserver::PersistentConfig config;
    config.filteringEnabled = true;
    config.channels["3491"] = {mw::log::LogLevel::kVerbose};
    config.defaultThreshold = mw::log::LogLevel::kVerbose;
    config.channelAssignments[dltid_t("000")][dltid_t("111")].push_back(dltid_t("22222"));
    config.messageThresholds[dltid_t("000")][dltid_t("111")] = mw::log::LogLevel::kVerbose;
    EXPECT_CALL(pd, setString(StrEq("dltConfig"), expected_json)).Times(1);
    writeDlt(config, pd);
}

// defaultThresold typo tests
// there is a typo in config file, defaultThresold instead of defaultThreshold
// we need to support both values, but take defaultThreshold as a primary if present

TEST(SocketserverConfigTest, DefaultThresholdValuePresentTakeIt)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-default-threshold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kDebug;
    EXPECT_EQ(result.value().defaultThreshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, DefaultThresoldValuePresentTakeIt)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-default-thresold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kDebug;
    EXPECT_EQ(result.value().defaultThreshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, BothValuesPresentTakeDefaultThreshold)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-thresold-and-threshold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kInfo;
    EXPECT_EQ(result.value().defaultThreshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, NoValuesSetkVerboseAsDefault)
{
    const auto result = readStaticDlt(PrepareLogChannelsPath("log-channels-no-default-threshold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kVerbose;
    EXPECT_EQ(result.value().defaultThreshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, GetStringCallExpected)
{
    StubPersistentDictionary pd;
    const std::string json{};
    auto default_return = pd.getString(CONFIG_DATABASE_KEY, json);
    ASSERT_EQ(default_return, json);
}

TEST(SocketserverConfigTest, GetBoolCallExpected)
{
    StubPersistentDictionary pd;
    const std::string key{};
    const bool defaultBoolValue{};
    auto default_return = pd.getBool(key, defaultBoolValue);
    ASSERT_EQ(default_return, defaultBoolValue);
}

}  // namespace
}  // namespace datarouter
}  // namespace platform
}  // namespace score
