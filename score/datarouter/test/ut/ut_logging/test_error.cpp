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

#include "gtest/gtest.h"
#include "score/datarouter/error/error.h"

namespace score
{
namespace logging
{
namespace error
{
namespace test
{

TEST(LoggingErrorDomainTest, TestkNoFileFoundCode)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Interface test");
    RecordProperty("Description", "Verify receiving 'No file was found' message for the error code kNoFileFound.");
    RecordProperty("DerivationTechnique", "Error guessing based on knowledge or experience");

    std::string_view expected_result("No file was found");
    LoggingErrorDomain obj;
    EXPECT_EQ(obj.MessageFor(static_cast<score::result::ErrorCode>(LoggingErrorCode::kNoFileFound)), expected_result);
}

TEST(LoggingErrorDomainTest, TestkParseErrorCode)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Interface test");
    RecordProperty("Description",
                   "Verify receiving 'Error when try to parse json file' message for the error"
                   "code kParseError.");
    RecordProperty("DerivationTechnique", "Error guessing based on knowledge or experience");

    std::string_view expected_result("Error when try to parse json file");
    LoggingErrorDomain obj;
    EXPECT_EQ(obj.MessageFor(static_cast<score::result::ErrorCode>(LoggingErrorCode::kParseError)), expected_result);
}

TEST(LoggingErrorDomainTest, TestkNoChannelsFoundCode)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Interface test");
    RecordProperty("Description",
                   "Verify receiving 'No channels information found' message for the error"
                   "code kNoChannelsFound");
    RecordProperty("DerivationTechnique", "Error guessing based on knowledge or experience");

    std::string_view expected_result("No channels information found");
    LoggingErrorDomain obj;
    EXPECT_EQ(obj.MessageFor(static_cast<score::result::ErrorCode>(LoggingErrorCode::kNoChannelsFound)), expected_result);
}

enum ExtendLoggingErrorCode : std::underlying_type_t<LoggingErrorCode>
{
    EXTENDED_CODE
};

TEST(LoggingErrorDomainTest, TestUnknownCode)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Interface test");
    RecordProperty("Description", "Verify receiving 'Unknown generic error' message for unknown error codes.");
    RecordProperty("DerivationTechnique", "Analysis of boundary values");

    std::string_view expected_result("Unknown generic error");
    LoggingErrorDomain obj;
    LoggingErrorCode custom_logging_error_code = static_cast<LoggingErrorCode>(ExtendLoggingErrorCode::EXTENDED_CODE);

    EXPECT_EQ(obj.MessageFor(static_cast<score::result::ErrorCode>(custom_logging_error_code)), expected_result);
}

}  // namespace test
}  // namespace error
}  // namespace logging
}  // namespace score
