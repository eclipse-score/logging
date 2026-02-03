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

#include "score/datarouter/include/daemon/verbose_dlt.h"

using namespace testing;
using namespace score::logging::dltserver;

class MockDltVerboseHandlerOutput : public DltVerboseHandler::IOutput
{
  public:
    MOCK_METHOD(void,
                SendVerbose,
                (uint32_t, const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection&),
                (override));
    virtual ~MockDltVerboseHandlerOutput() = default;
};

TEST(DltVerboseHandlerTest, sendVerboseTest)
{
    MockDltVerboseHandlerOutput mock_dlt_output;
    DltVerboseHandler handler(mock_dlt_output);

    const TimestampT timestamp = score::os::HighResolutionSteadyClock::time_point{};
    const char* data = "data";
    const BufsizeT data_size = static_cast<BufsizeT>(strlen(data));

    EXPECT_CALL(mock_dlt_output, SendVerbose(_, _)).Times(1);

    handler.Handle(timestamp, data, data_size);
}
