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
#include "score/mw/log/detail/common/recorder_factory.h"

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

    std::unique_ptr<Recorder> CreateFromConfiguration() noexcept
    {
        auto config_reader_mock = std::make_unique<TargetConfigReaderMock>();
        ON_CALL(*config_reader_mock, ReadConfig).WillByDefault(testing::Invoke([&]() {
            return config_result_;
        }));
        return RecorderFactory{}.CreateFromConfiguration(std::move(config_reader_mock),
                                                         score::cpp::pmr::get_default_resource());
    }

    void SetTargetConfigReaderResult(score::Result<Configuration> result) noexcept
    {
        config_result_ = result;
    }

    void SetConfigurationWithLogMode(const std::unordered_set<LogMode>& log_modes,
                                     Configuration config = Configuration{}) noexcept
    {
        config.SetLogMode(log_modes);
        SetTargetConfigReaderResult(config);
    }

  protected:
    score::Result<Configuration> config_result_;
    score::cpp::pmr::memory_resource* memory_resource_ = nullptr;
};

TEST_F(RecorderFactoryConfigFixture, RemoteConfiguredShallReturnDataRouterRecorder)
{
    RecordProperty("Requirement", "SCR-861534");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "RecorderFactory can create DataRouterRecorder if remote is configured.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SetConfigurationWithLogMode({LogMode::kRemote});
    auto recorder = CreateFromConfiguration();
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

    SetConfigurationWithLogMode({LogMode::kFile, LogMode::kConsole, LogMode::kRemote});
    auto recorder = CreateFromConfiguration();
    ASSERT_TRUE(IsRecorderOfType<CompositeRecorder>(recorder));
    const auto& composite_recorder = *dynamic_cast<CompositeRecorder*>(recorder.get());

    EXPECT_EQ(composite_recorder.GetRecorders().size(), config_result_->GetLogMode().size());
    EXPECT_TRUE(ContainsRecorderOfType<DataRouterRecorder>(composite_recorder));
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
