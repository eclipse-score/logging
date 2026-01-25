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

namespace score
{
namespace platform
{
namespace internal
{
PersistentLoggingConfig gPerLogConfOk;
PersistentLoggingConfig gPerLogConfErrOpn;
PersistentLoggingConfig gPerLogConfErrParse;
PersistentLoggingConfig gPerLogConfErrContent1;
PersistentLoggingConfig gPerLogConfErrContent2;
PersistentLoggingConfig gPerLogConfErrContent3;
PersistentLoggingConfig gPerLogConfErrContent4;
PersistentLoggingConfig gPerLogConfErrContent5;
using VerbFilterType = std::vector<std::tuple<std::string, std::string, uint8_t>>;

const VerbFilterType kOkVerboseFilters = {{"", "", 2},
                                          {"CDH", "SHCD", 4},
                                          {"CHD", "DFLT", 4},
                                          {"LOGC", "", 5},
                                          {"EM", "prlf", 5},
                                          {"MSM", "MSM", 5},
                                          {"MON", "CPUS", 4},
                                          {"MON", "MEMS", 4},
                                          {"UTC", "UTC", 4}};

const std::vector<std::string> kOkNonVerboseFilters = {"score::tracing::TimeTrace",
                                                       "aas::logging::ReprocessingCycle",
                                                       "score::logging::standard_frame::CurrentEngineeringMode",
                                                       "score::logging::standard_frame::EcuHwVersion",
                                                       "score::logging::standard_frame::EcuSwVersion1",
                                                       "score::logging::standard_frame::EcuSwVersion2",
                                                       "score::logging::standard_frame::EcuSwVersion3",
                                                       "score::logging::standard_frame::EcuSwVersion4",
                                                       "score::logging::standard_frame::EcuSwVersion5",
                                                       "score::logging::standard_frame::EcuSwVersion6"};

std::uint16_t gRoutineId;

class DltSetLogLevelTest : public ::testing::Test
{
  protected:
    virtual void SetUp() override
    {
        const std::string complete_test_path = "score/datarouter/test/ut/etc/datarouter/";

        gPerLogConfOk = readPersistentLoggingConfig(complete_test_path + std::string("persistent-logging.json"));
        gPerLogConfErrOpn = readPersistentLoggingConfig("persistent-ln");
        gPerLogConfErrParse =
            readPersistentLoggingConfig(complete_test_path + std::string("persistent-logging_test_1.json"));
        gPerLogConfErrContent1 =
            readPersistentLoggingConfig(complete_test_path + std::string("persistent-logging_test_2.json"));
        gPerLogConfErrContent2 =
            readPersistentLoggingConfig(complete_test_path + std::string("persistent-logging_test_3.json"));
        gPerLogConfErrContent3 =
            readPersistentLoggingConfig(complete_test_path + std::string("persistent-logging_test_4.json"));
        gPerLogConfErrContent4 =
            readPersistentLoggingConfig(complete_test_path + std::string("persistent-logging_test_5.json"));
        gPerLogConfErrContent5 =
            readPersistentLoggingConfig(complete_test_path + std::string("persistent-logging_test_6.json"));
    }
    virtual void TearDown() override {}
};

TEST_F(DltSetLogLevelTest, JSON_OK)
{
    EXPECT_TRUE(gPerLogConfOk.readResult_ == score::platform::internal::PersistentLoggingConfig::ReadResult::OK);
    auto& verbose_filters = gPerLogConfOk.verboseFilters_;
    auto& non_verbose_filters = gPerLogConfOk.nonVerboseFilters_;
    VerbFilterType conv_verbose_filter;
    for (const auto& filter : verbose_filters)
    {
        char temp_buf[8] = {0};
        std::memcpy(temp_buf, filter.appid_.GetStringView().data(), 4);
        std::string app_id(temp_buf);
        std::memcpy(temp_buf, filter.ctxid_.GetStringView().data(), 4);
        std::string ctxid(temp_buf);
        conv_verbose_filter.emplace_back(app_id, ctxid, filter.logLevel_);
    }

    using LmbTpl = std::tuple<const std::string, const std::string, const int>;
    EXPECT_TRUE(std::equal(conv_verbose_filter.cbegin(),
                           conv_verbose_filter.cend(),
                           kOkVerboseFilters.cbegin(),
                           kOkVerboseFilters.cend(),
                           [](const LmbTpl& lhs, const LmbTpl& rhs) -> bool {
                               return (std::get<0>(lhs) == std::get<0>(rhs) && std::get<1>(lhs) == std::get<1>(rhs) &&
                                       std::get<2>(lhs) == std::get<2>(rhs));
                           }));
    EXPECT_TRUE(std::equal(non_verbose_filters.cbegin(),
                           non_verbose_filters.cend(),
                           kOkNonVerboseFilters.cbegin(),
                           kOkNonVerboseFilters.cend(),
                           [](const std::string& lhs, const std::string& rhs) -> bool {
                               return (lhs == rhs);
                           }));
}

TEST_F(DltSetLogLevelTest, NO_JSON_FILE)
{
    EXPECT_TRUE(gPerLogConfErrOpn.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_OPEN);
}

TEST_F(DltSetLogLevelTest, JSON_FILE_ERROR)
{
    EXPECT_TRUE(gPerLogConfErrParse.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_PARSE);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_NO_FILTERS)
{
    EXPECT_TRUE(gPerLogConfErrContent1.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_VERBOSE_FILTERS)
{
    EXPECT_TRUE(gPerLogConfErrContent2.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_VERBOSE_FILTERS_NOT_STRING)
{
    EXPECT_TRUE(gPerLogConfErrContent3.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_NON_VERBOSE_FILTERS_NOT_STRING)
{
    EXPECT_TRUE(gPerLogConfErrContent4.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

TEST_F(DltSetLogLevelTest, JSON_ERROR_NON_VERBOSE_FILTERS)
{
    EXPECT_TRUE(gPerLogConfErrContent5.readResult_ ==
                score::platform::internal::PersistentLoggingConfig::ReadResult::ERROR_CONTENT);
}

}  // namespace internal
}  // namespace platform
}  // namespace score
