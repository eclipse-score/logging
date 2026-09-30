/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#include "score/mw/log/backend_table.h"

#include "gtest/gtest.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
using namespace score::mw::log::detail;  // NOLINT(google-build-using-namespace) keep names unqualified below
using namespace score::mw::log;          // NOLINT(google-build-using-namespace) keep names unqualified below

namespace
{

TEST(SyslogRegistrantTest, SyslogBackendIsRegisteredAfterStaticInitialization)
{
    RecordProperty("PartiallyVerifies", "comp_req__log__system_backend_activation");
    RecordProperty("Description",
                   "The syslog backend registrant is registered for LogMode::kSystem during static init.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("lobster-tracing", "MwLog.SystemBackendActivation");

    EXPECT_TRUE(IsBackendAvailable(LogMode::kSystem));
}

TEST(SyslogRegistrantTest, RegisteredCreatorProducesARecorder)
{
    RecordProperty("PartiallyVerifies", "comp_req__log__system_backend_activation");
    RecordProperty("Description",
                   "The creator function registered for LogMode::kSystem actually builds a usable Recorder.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");
    RecordProperty("lobster-tracing", "MwLog.SystemBackendActivation");

    Configuration config{};
    const auto recorder = CreateRecorderForMode(LogMode::kSystem, config, score::cpp::pmr::get_default_resource());

    EXPECT_NE(recorder, nullptr);
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
