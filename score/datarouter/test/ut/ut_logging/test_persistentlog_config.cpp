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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

#include "daemon/persistentlogging_config.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

namespace score
{
namespace platform
{
namespace internal
{
PersistentLoggingConfig perLogConf_OK;
PersistentLoggingConfig perLogConf_ERR_OPN;
PersistentLoggingConfig perLogConf_ERR_PARSE;
PersistentLoggingConfig perLogConf_ERR_CONTENT_1;
PersistentLoggingConfig perLogConf_ERR_CONTENT_2;
PersistentLoggingConfig perLogConf_ERR_CONTENT_3;
PersistentLoggingConfig perLogConf_ERR_CONTENT_4;
PersistentLoggingConfig perLogConf_ERR_CONTENT_5;
using verbFilterType = std::vector<std::tuple<std::string, std::string, uint8_t>>;

const verbFilterType okVerboseFilters = {{"", "", 2},
                                         {"CDH", "SHCD", 4},
                                         {"CHD", "DFLT", 4},
                                         {"LOGC", "", 5},
                                         {"EM", "prlf", 5},
                                         {"MSM", "MSM", 5},
                                         {"MON", "CPUS", 4},
                                         {"MON", "MEMS", 4},
                                         {"UTC", "UTC", 4}};

const std::vector<std::string> okNonVerboseFilters = {"score::tracing::TimeTrace",
                                                      "aas::logging::ReprocessingCycle",
                                                      "score::logging::standard_frame::CurrentEngineeringMode",
                                                      "score::logging::standard_frame::EcuHwVersion",
                                                      "score::logging::standard_frame::EcuSwVersion1",
                                                      "score::logging::standard_frame::EcuSwVersion2",
                                                      "score::logging::standard_frame::EcuSwVersion3",
                                                      "score::logging::standard_frame::EcuSwVersion4",
                                                      "score::logging::standard_frame::EcuSwVersion5",
                                                      "score::logging::standard_frame::EcuSwVersion6"};

std::uint16_t routine_id;

class DltSetLogLevelTest : public ::testing::Test
{
  protected:
    virtual void SetUp() override
    {
        const std::string completeTestPath = "score/datarouter/test/ut/etc/datarouter/";

        perLogConf_OK = readPersistentLoggingConfig(completeTestPath + std::string("persistent-logging.json"));
        perLogConf_ERR_OPN = readPersistentLoggingConfig("persistent-ln");
        perLogConf_ERR_PARSE =
            readPersistentLoggingConfig(completeTestPath + std::string("persistent-logging_test_1.json"));
        perLogConf_ERR_CONTENT_1 =
            readPersistentLoggingConfig(completeTestPath + std::string("persistent-logging_test_2.json"));
        perLogConf_ERR_CONTENT_2 =
            readPersistentLoggingConfig(completeTestPath + std::string("persistent-logging_test_3.json"));
        perLogConf_ERR_CONTENT_3 =
            readPersistentLoggingConfig(completeTestPath + std::string("persistent-logging_test_4.json"));
        perLogConf_ERR_CONTENT_4 =
            readPersistentLoggingConfig(completeTestPath + std::string("persistent-logging_test_5.json"));
        perLogConf_ERR_CONTENT_5 =
            readPersistentLoggingConfig(completeTestPath + std::string("persistent-logging_test_6.json"));
    }
    virtual void TearDown() override {}
};

TEST_F(DltSetLogLevelTest, JSON_OK)
{
    EXPECT_TRUE(perLogConf_OK.readResult_ == score::platform::internal::PersistentLoggingConfig::ReadResult::OK);
    auto& verboseFilters = perLogConf_OK.verboseFilters_;
    auto& nonVerboseFilters = perLogConf_OK.nonVerboseFilters_;
    verbFilterType convVerboseFilter;
    for (const auto& filter : verboseFilters)
    {
        char tempBuf[8] = {0};
        std::memcpy(tempBuf, filter.appid_.GetStringView().data(), 4);
        std::string appId(tempBuf);
        std::memcpy(tempBuf, filter.ctxid_.GetStringView().data(), 4);
        std::string ctxid(tempBuf);
        convVerboseFilter.emplace_back(appId, ctxid, filter.logLevel_);
    }

    using lmbTpl = std::tuple<const std::string, const std::string, const int>;
    EXPECT_TRUE(std::equal(convVerboseFilter.cbegin(),
                           convVerboseFilter.cend(),
                           okVerboseFilters.cbegin(),
                           okVerboseFilters.cend(),
                           [](const lmbTpl& lhs, const lmbTpl& rhs) -> bool {
                               return (std::get<0>(lhs).compare(std::get<0>(rhs)) == 0 &&
                                       std::get<1>(lhs).compare(std::get<1>(rhs)) == 0 &&
                                       std::get<2>(lhs) == std::get<2>(rhs));
                           }));
    EXPECT_TRUE(std::equal(nonVerboseFilters.cbegin(),
                           nonVerboseFilters.cend(),
                           okNonVerboseFilters.cbegin(),
                           okNonVerboseFilters.cend(),
                           [](const std::string& lhs, const std::string& rhs) -> bool {
                               return (lhs.compare(rhs) == 0);
                           }));
}

TEST_F(DltSetLogLevelTest, NO_JSON_FILE)
{
    EXPECT_TRUE(perLogConf_ERR_OPN.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_OPEN);
}

TEST_F(DltSetLogLevelTest, JSON_FILE_ERROR)
{
    EXPECT_TRUE(perLogConf_ERR_PARSE.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_PARSE);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_NO_FILTERS)
{
    EXPECT_TRUE(perLogConf_ERR_CONTENT_1.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_VERBOSE_FILTERS)
{
    EXPECT_TRUE(perLogConf_ERR_CONTENT_2.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_VERBOSE_FILTERS_NOT_STRING)
{
    EXPECT_TRUE(perLogConf_ERR_CONTENT_3.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_NON_VERBOSE_FILTERS_NOT_STRING)
{
    EXPECT_TRUE(perLogConf_ERR_CONTENT_4.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_NON_VERBOSE_FILTERS)
{
    EXPECT_TRUE(perLogConf_ERR_CONTENT_5.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

}  // namespace internal
}  // namespace platform
}  // namespace score
