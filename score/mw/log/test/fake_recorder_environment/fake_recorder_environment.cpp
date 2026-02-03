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
#include "score/mw/log/test/fake_recorder_environment/fake_recorder_environment.h"

#include "score/mw/log/runtime.h"

namespace score
{
namespace mw
{
namespace log
{
namespace test
{

void FakeRecorderEnvironment::SetUp()
{
    // Save current recorder (if any) before installing fake
    previous_recorder_ = &score::mw::log::detail::Runtime::GetRecorder();

    // Create and install fake recorder
    recorder_ = std::make_unique<FakeRecorder>();
    score::mw::log::detail::Runtime::SetRecorder(recorder_.get());
}

void FakeRecorderEnvironment::TearDown()
{
    // Restore previous recorder (or nullptr)
    score::mw::log::detail::Runtime::SetRecorder(previous_recorder_);
    recorder_.reset();
    previous_recorder_ = nullptr;
}

}  // namespace test
}  // namespace log
}  // namespace mw
}  // namespace score
