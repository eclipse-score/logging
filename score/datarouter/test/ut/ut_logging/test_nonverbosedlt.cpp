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
#include "score/datarouter/nonverbose_dlt/nonverbose_dlt.h"

using namespace ::testing;
using namespace score::logging::dltserver;

namespace score
{
namespace platform
{
namespace datarouter
{
namespace test
{

struct TestTraceableStruct
{
    std::uint32_t i;
    std::uint32_t j;
};
STRUCT_TRACEABLE(TestTraceableStruct, i, j)

}  // namespace test
}  // namespace datarouter
}  // namespace platform
}  // namespace score

class MockDltOutput : public DltNonverboseHandler::IOutput
{
  public:
    MOCK_METHOD(void,
                sendNonVerbose,
                (const score::mw::log::config::NvMsgDescriptor& desc, uint32_t tmsp, const void* data, size_t size),
                (override));
    virtual ~MockDltOutput() = default;
};

class DltNonverboseHandlerTest : public ::testing::Test
{
  protected:
    MockDltOutput mockOutput;
    std::unique_ptr<DltNonverboseHandler> handler;
    DltNonverboseHandlerTest() = default;

    void SetUp() override
    {
        handler = std::make_unique<DltNonverboseHandler>(mockOutput);
    }
};

TEST_F(DltNonverboseHandlerTest, HandleShouldCallSendNonVerbose)
{
    TypeInfo typeInfo;
    typeInfo.typeName = "score::platform::datarouter::test::TestTraceableStruct";
    timestamp_t timestamp = score::os::HighResolutionSteadyClock::now();
    const char* data = "TestData";
    bufsize_t size = 10;

    score::mw::log::config::NvMsgDescriptor descriptor;
    typeInfo.nvMsgDesc = &descriptor;

    handler->handle(typeInfo, timestamp, data, size);
}

TEST(DltNonverboseHandler_T, HandleShouldNotCallSendNonVerboseWhenDescriptorIsNull)
{
    TypeInfo typeInfo;
    typeInfo.typeName = "score::platform::datarouter::test::TestTraceableStruct";
    typeInfo.nvMsgDesc = nullptr;
    timestamp_t timestamp = score::os::HighResolutionSteadyClock::now();
    const char data[] = "TestLogData";
    bufsize_t size = sizeof(data);

    MockDltOutput mockOutput;
    DltNonverboseHandler handler(mockOutput);

    EXPECT_CALL(mockOutput, sendNonVerbose(_, _, _, _)).Times(0);

    handler.handle(typeInfo, timestamp, data, size);
}

TEST(DltNonverboseHandler_T, HandleCallSendNonVerboseWhenDltMsgDesc)
{
    MockDltOutput mockOutput;

    static const score::mw::log::config::NvMsgDescriptor descriptor{1234U,
                                                                  score::mw::log::detail::LoggingIdentifier{"APP0"},
                                                                  score::mw::log::detail::LoggingIdentifier{"CTX0"},
                                                                  score::mw::log::LogLevel::kOff};

    EXPECT_CALL(mockOutput, sendNonVerbose(_, _, _, _)).Times(1);

    DltNonverboseHandler handler(mockOutput);

    TypeInfo typeInfo;
    typeInfo.typeName = "score::platform::datarouter::test::TestTraceableStruct";
    typeInfo.nvMsgDesc = &descriptor;

    timestamp_t timestamp = score::os::HighResolutionSteadyClock::now();
    const char data[] = "TestData";
    bufsize_t size = sizeof(data);
    handler.handle(typeInfo, timestamp, data, size);
}
