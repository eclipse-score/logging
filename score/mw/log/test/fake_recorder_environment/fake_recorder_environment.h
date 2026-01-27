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
#ifndef SCORE_MW_LOG_TEST_FAKE_RECORDER_ENVIRONMENT_FAKE_RECORDER_ENVIRONMENT_H
#define SCORE_MW_LOG_TEST_FAKE_RECORDER_ENVIRONMENT_FAKE_RECORDER_ENVIRONMENT_H

#include "score/mw/log/test/fake_recorder/fake_recorder.h"

#include <gtest/gtest.h>

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace test
{

class FakeRecorderEnvironment : public ::testing::Environment
{
  public:
    FakeRecorderEnvironment() noexcept = default;
    ~FakeRecorderEnvironment() noexcept override = default;

    FakeRecorderEnvironment(const FakeRecorderEnvironment&) = delete;
    FakeRecorderEnvironment& operator=(const FakeRecorderEnvironment&) = delete;
    FakeRecorderEnvironment(FakeRecorderEnvironment&&) = delete;
    FakeRecorderEnvironment& operator=(FakeRecorderEnvironment&&) = delete;

    void SetUp() override;
    void TearDown() override;

  private:
    std::unique_ptr<FakeRecorder> recorder_;
    score::mw::log::Recorder* previous_recorder_{nullptr};
};

}  // namespace test
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_TEST_FAKE_RECORDER_ENVIRONMENT_FAKE_RECORDER_ENVIRONMENT_H
