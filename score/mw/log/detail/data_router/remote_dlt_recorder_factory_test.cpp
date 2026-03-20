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

#include "score/mw/log/detail/data_router/data_router_recorder.h"

#include "score/mw/log/detail/common/composite_recorder.h"
#include "score/mw/log/detail/registry_aware_recorder_factory.h"

#include "score/mw/log/configuration/target_config_reader_mock.h"

#include "score/mw/log/detail/empty_recorder.h"
#include "score/mw/log/detail/error.h"

#include "gtest/gtest.h"

#include <type_traits>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
namespace
{

template <typename ConcreteRecorder>
bool IsRecorderOfType(const std::unique_ptr<Recorder>& recorder) noexcept
{
    static_assert(std::is_base_of<Recorder, ConcreteRecorder>::value,
                  "Concrete recorder shall be derived from Recorder base class");

    return dynamic_cast<const ConcreteRecorder*>(recorder.get()) != nullptr;
}

template <typename ConcreteRecorder>
bool ContainsRecorderOfType(const CompositeRecorder& composite) noexcept
{
    bool return_value = false;
    for (const auto& recorder : composite.GetRecorders())
    {
        if (IsRecorderOfType<ConcreteRecorder>(recorder))
        {
            return_value = true;
            break;
        }
    }
    return return_value;
}

class RecorderFactoryConfigFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        memory_resource_ = score::cpp::pmr::get_default_resource();
    }
    void TearDown() override {}

  protected:
    score::cpp::pmr::memory_resource* memory_resource_ = nullptr;
};

TEST_F(RecorderFactoryConfigFixture, RemoteConfiguredShallReturnDataRouterRecorder)
{
    RecordProperty("Requirement", "SCR-861534");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "RecorderFactory can create DataRouterRecorder if remote is configured.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const Configuration config{};
    auto recorder =
        RegistryAwareRecorderFactory{}.CreateRecorderFromLogMode(LogMode::kRemote, config, memory_resource_);
    EXPECT_TRUE(IsRecorderOfType<DataRouterRecorder>(recorder));
}

TEST_F(RecorderFactoryConfigFixture, MultipleLogModesShallReturnCompositeRecorder)
{
    RecordProperty("Requirement", "SCR-861534");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "RecorderFactory shall create CompositeRecorder in case of multiple log mode is configured.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const Configuration config{};
    const std::unordered_set<LogMode> modes{LogMode::kFile, LogMode::kConsole, LogMode::kRemote};

    std::vector<std::unique_ptr<Recorder>> recorders;
    for (const auto& mode : modes)
    {
        auto recorder = RegistryAwareRecorderFactory{}.CreateRecorderFromLogMode(mode, config, memory_resource_);
        if (recorder != nullptr)
        {
            std::ignore = recorders.emplace_back(std::move(recorder));
        }
    }

    ASSERT_EQ(recorders.size(), modes.size());
    auto composite = std::make_unique<CompositeRecorder>(std::move(recorders));
    EXPECT_TRUE(ContainsRecorderOfType<DataRouterRecorder>(*composite));
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
