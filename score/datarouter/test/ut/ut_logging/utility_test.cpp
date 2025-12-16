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

#include "score/datarouter/include/daemon/utility.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(UtilityTest, log_levels)
{
    EXPECT_EQ("kVerbose", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("kVerbose")));
    EXPECT_EQ("kDebug", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("kDebug")));
    EXPECT_EQ("kInfo", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("kInfo")));
    EXPECT_EQ("kWarn", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("kWarn")));
    EXPECT_EQ("kError", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("kError")));
    EXPECT_EQ("kFatal", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("kFatal")));
    EXPECT_EQ("kOff", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("kOff")));
    //  Hacky way to achieve out-of range enum value:
    uint8_t value = uint8_t{255UL};
    EXPECT_EQ("undefined",
              logchannel_operations ::GetStringFromLogLevel(*reinterpret_cast<score::mw::log::LogLevel*>(&value)));

    EXPECT_EQ("kOff", logchannel_operations ::GetStringFromLogLevel(logchannel_operations::ToLogLevel("undefined")));
}
