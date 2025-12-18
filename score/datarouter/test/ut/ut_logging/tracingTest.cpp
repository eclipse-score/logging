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

#ifndef AAS_PAS_LOGGING_TRACING_TEST_H
#define AAS_PAS_LOGGING_TRACING_TEST_H

#include "gtest/gtest.h"
#include <Tracing>
// #include "appconfig_mock.h"

namespace score
{
namespace logging
{

class TracingTest : public ::testing::Test
{
  protected:
    virtual void SetUp() override {}
    virtual void TearDown() override {}
};

TEST_F(TracingTest, TracingLifeCycleStart)
{
    // EXPECT_CALL(appconfigmock_, getApplicationId().Times(AtLeast(1)).WillOnce(Return("SMOKE"));
    //--TODO expectations need to be added after the below mocks are available
    // score::platform::internal::UnixDomainSockAddr
    // score::platform::logger
    // score::platform::internal::MwsrDataReadAcquired
    // score::platform::internal::MwsrDataReadReleased
    // Currently running with original lib_tracing library
}
}  // namespace logging
}  // namespace score

#endif  // AAS_PAS_LOGGING_TRACING_TEST_H
