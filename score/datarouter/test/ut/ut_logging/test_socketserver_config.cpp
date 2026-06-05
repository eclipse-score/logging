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

const std::string kConfigDatabaseKey = "dltConfig";
const std::string kConfigOutputEnabledKey = "dltOutputEnabled";

std::string PrepareLogChannelsPath(const std::string& file_name)
{
    const std::string override_path = "score/datarouter/test/ut/etc/" + file_name;
    return override_path;
}

// ReadStaticDlt unit tests

TEST(SocketserverConfigTest, ReadCorrectLogChannelsNoErrorsExpected)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels.json").c_str());
    EXPECT_TRUE(result.has_value());
    EXPECT_THAT(result.value().channels, SizeIs(3));
    EXPECT_THAT(result.value().channel_assignments, SizeIs(2));
    EXPECT_THAT(result.value().message_thresholds, SizeIs(3));
    EXPECT_TRUE(result.value().filtering_enabled);
}

TEST(SocketserverConfigTest, ReadNonExistingPathErrorExpected)
{
    const auto result = ReadStaticDlt("");
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, ReadEmptyLogChannelErrorExpected)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-empty.json").c_str());
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, JsonWithoutChannelsErrorExpected)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-without-channels.json").c_str());
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, JsonEmptyChannelsErrorExpected)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-empty-channels.json").c_str());
    EXPECT_FALSE(result.has_value());
}

TEST(SocketserverConfigTest, JsonFilteringEnabledExpectConfigFilterTrue)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-filtering-enabled.json").c_str());
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().filtering_enabled);
}

TEST(SocketserverConfigTest, JsonQuotasEnabledExpectConfigFilterTrue)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-quotas.json").c_str());
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().throughput.overall_mbps, 100);
    EXPECT_FALSE(result.value().quota_enforcement_enabled);
    EXPECT_THAT(result.value().throughput.applications_kbps, SizeIs(1));
}

TEST(SocketserverConfigTest, JsonQuotasEnabledActivated)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-quotas-activated.json").c_str());
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().quota_enforcement_enabled);
}

TEST(SocketserverConfigTest, JsonQuotasEnabledDeactivated)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-quotas-deactivated.json").c_str());
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().quota_enforcement_enabled);
}

TEST(SocketserverConfigTest, JsonOldFormatErrorExpected)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-old-format.json").c_str());
    EXPECT_FALSE(result.has_value());
}

// ReadDlt unit tests

TEST(SocketserverConfigTest, PersistentDictionaryEmptyJsonErrorExpected)
{
    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, GetString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return("{}"));
    const auto result = ReadDlt(pd);
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
    EXPECT_CALL(pd, GetString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return(expected_json));
    const auto result = ReadDlt(pd);
    EXPECT_TRUE(result.filtering_enabled);
    EXPECT_THAT(result.channels, SizeIs(3));
    EXPECT_THAT(result.channel_assignments, SizeIs(2));
    EXPECT_THAT(result.message_thresholds, SizeIs(3));
}

TEST(SocketserverConfigTest, PersistentDictionaryEmptyChannelsErrorExpected)
{
    const std::string expected_json{
        "{\"channels\":{},\"channelAssignments\":{\"DR\":{\"\":[\"3492\"],\"CTX1\":[\"3492\",\"3493\"]},\"-NI-\":{\"\":"
        "[\"3491\"]}},\"filteringEnabled\":true,\"defaultChannel\":\"3493\",\"defaultThresold\":\"kVerbose\","
        "\"messageThresholds\":{\"\":{\"vcip\":\"kInfo\"},\"DR\":{\"\":\"kVerbose\",\"CTX1\":\"kVerbose\",\"STAT\":"
        "\"kDebug\"},\"-NI-\":{\"\":\"kVerbose\"}}}"};

    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, GetString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return(expected_json));
    const auto result = ReadDlt(pd);
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
    EXPECT_CALL(pd, GetString(StrEq("dltConfig"), "{}")).Times(1).WillOnce(Return(expected_json));
    const auto result = ReadDlt(pd);
    EXPECT_TRUE(result.filtering_enabled);
}

// WriteDltEnabled unit test

TEST(SocketserverConfigTest, WriteDltEnabledCallSetBoolExpected)
{
    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, SetBool(StrEq("dltOutputEnabled"), true)).Times(1);
    WriteDltEnabled(true, pd);
}

// ReadDltEnabled unit test

TEST(SocketserverConfigTest, ReadDltEnabledTrueResultExpected)
{
    StrictMock<MockPersistentDictionary> pd;
    EXPECT_CALL(pd, GetBool(StrEq("dltOutputEnabled"), true)).WillOnce(Return(true));
    const auto result = ReadDltEnabled(pd);
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
    config.filtering_enabled = true;
    config.channels["3491"] = {mw::log::LogLevel::kVerbose};
    config.default_threshold = mw::log::LogLevel::kVerbose;
    config.channel_assignments[DltidT("000")][DltidT("111")].push_back(DltidT("22222"));
    config.message_thresholds[DltidT("000")][DltidT("111")] = mw::log::LogLevel::kVerbose;
    EXPECT_CALL(pd, SetString(StrEq("dltConfig"), expected_json)).Times(1);
    WriteDlt(config, pd);
}

// defaultThresold typo tests
// there is a typo in config file, defaultThresold instead of defaultThreshold
// we need to support both values, but take defaultThreshold as a primary if present

TEST(SocketserverConfigTest, DefaultThresholdValuePresentTakeIt)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-default-threshold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kDebug;
    EXPECT_EQ(result.value().default_threshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, DefaultThresoldValuePresentTakeIt)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-default-thresold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kDebug;
    EXPECT_EQ(result.value().default_threshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, BothValuesPresentTakeDefaultThreshold)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-thresold-and-threshold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kInfo;
    EXPECT_EQ(result.value().default_threshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, NoValuesSetkVerboseAsDefault)
{
    const auto result = ReadStaticDlt(PrepareLogChannelsPath("log-channels-no-default-threshold.json").c_str());
    EXPECT_TRUE(result.has_value());
    const auto expected_log_level_threshold = mw::log::LogLevel::kVerbose;
    EXPECT_EQ(result.value().default_threshold, expected_log_level_threshold);
}

TEST(SocketserverConfigTest, GetStringCallExpected)
{
    StubPersistentDictionary pd;
    const std::string json{};
    auto default_return = pd.GetString(kConfigDatabaseKey, json);
    ASSERT_EQ(default_return, json);
}

TEST(SocketserverConfigTest, GetBoolCallExpected)
{
    StubPersistentDictionary pd;
    const std::string key{};
    const bool default_bool_value{};
    auto default_return = pd.GetBool(key, default_bool_value);
    ASSERT_EQ(default_return, default_bool_value);
}

}  // namespace
}  // namespace datarouter
}  // namespace platform
}  // namespace score
