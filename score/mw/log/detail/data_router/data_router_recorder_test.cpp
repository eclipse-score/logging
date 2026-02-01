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

#include "gtest/gtest.h"

#include <limits>
#include <memory>

#include "score/mw/log/detail/backend_mock.h"

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

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

constexpr LogLevel kActiveLogLevel = LogLevel::kError;
constexpr LogLevel kInActiveLogLevel = LogLevel::kInfo;
static_assert(static_cast<std::underlying_type_t<LogLevel>>(kActiveLogLevel) <
                  static_cast<std::underlying_type_t<LogLevel>>(kInActiveLogLevel),
              "Log Level setup for this test makes no sense.");

class DataRouterRecorderFixtureWithDepletedBackend : public testing::Test
{
  public:
    void SetUp() override
    {
        EXPECT_CALL(*backend_, ReserveSlot()).WillOnce(Return(score::cpp::optional<SlotHandle>{}));
        recorder_ = std::make_unique<DataRouterRecorder>(std::move(backend_), config_);
    }

    void TearDown() override {}

  protected:
    Configuration config_{};
    std::unique_ptr<NiceMock<BackendMock>> backend_ = std::make_unique<NiceMock<BackendMock>>();
    std::unique_ptr<DataRouterRecorder> recorder_;
};

TEST_F(DataRouterRecorderFixtureWithDepletedBackend, WillReturnEmptySlot)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of reserving a slot with depleted backend.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    const auto slot = recorder_->StartRecord("any_id", kActiveLogLevel);
    EXPECT_FALSE(slot.has_value());
}

class DataRouterRecorderFixtureWithLogLevelCheck : public testing::Test
{
  public:
    void SetUp() override
    {
        ON_CALL(*backend_, ReserveSlot()).WillByDefault(Return(slot_));
        ON_CALL(*backend_, GetLogRecord(slot_)).WillByDefault(ReturnRef(log_record_));
        const ContextLogLevelMap context_log_level_map{{LoggingIdentifier{context_id_}, kActiveLogLevel}};
        config_.SetContextLogLevel(context_log_level_map);
        recorder_ = std::make_unique<DataRouterRecorder>(std::move(backend_), config_);
    }

    void TearDown() override {}

  protected:
    std::unique_ptr<NiceMock<BackendMock>> backend_ = std::make_unique<NiceMock<BackendMock>>();
    const std::string_view context_id_ = "DFLT";
    Configuration config_{};
    std::unique_ptr<DataRouterRecorder> recorder_;
    SlotHandle slot_{};
    LogRecord log_record_{};
};

TEST_F(DataRouterRecorderFixtureWithLogLevelCheck, WillObtainSlotForSufficientLogLevel)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of reserving a slot with proper context ID.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    const auto slot = recorder_->StartRecord(context_id_, kActiveLogLevel);
    EXPECT_TRUE(slot.has_value());
}

TEST_F(DataRouterRecorderFixtureWithLogLevelCheck, WillObtainEmptySlotForInsufficentLogLevel)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of returning a slot in case of inactive log level.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    const auto slot = recorder_->StartRecord(context_id_, kInActiveLogLevel);
    EXPECT_FALSE(slot.has_value());
}

TEST_F(DataRouterRecorderFixtureWithLogLevelCheck, DisablesOrEnablesLogAccordingToLevel)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of checking the log availability using 'IsLogEnabled' API.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_TRUE(recorder_->IsLogEnabled(kActiveLogLevel, context_id_));
    EXPECT_FALSE(recorder_->IsLogEnabled(kInActiveLogLevel, context_id_));
}

class DataRouterRecorderFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        EXPECT_CALL(*backend_, ReserveSlot()).WillOnce(Return(slot_));
        EXPECT_CALL(*backend_, FlushSlot(slot_));
        ON_CALL(*backend_, GetLogRecord(slot_)).WillByDefault(ReturnRef(log_record_));
        recorder_ = std::make_unique<DataRouterRecorder>(std::move(backend_), config_);
        recorder_->StartRecord(context_id_, log_level_);
    }

    void TearDown() override
    {
        const auto& log_entry = log_record_.getLogEntry();
        const auto config_app_id = config_.GetAppId();
        const auto log_entry_app_id = log_entry.app_id.GetStringView();
        EXPECT_EQ(config_app_id, log_entry_app_id);
        EXPECT_EQ(log_entry.ctx_id.GetStringView(), context_id_);
        EXPECT_EQ(log_entry.log_level, log_level_);
        EXPECT_EQ(log_entry.num_of_args, expected_number_of_arguments_at_teardown_);
        recorder_->StopRecord(slot_);
    }

  protected:
    Configuration config_{};
    std::unique_ptr<NiceMock<BackendMock>> backend_ = std::make_unique<NiceMock<BackendMock>>();
    std::unique_ptr<DataRouterRecorder> recorder_;
    SlotHandle slot_{};
    LogRecord log_record_{};
    LogLevel log_level_ = kActiveLogLevel;
    const std::string_view context_id_ = "DFLT";
    std::uint8_t expected_number_of_arguments_at_teardown_{1};
};

TEST_F(DataRouterRecorderFixture, TooManyArgumentsWillYieldTruncatedLog)
{
    RecordProperty("ASIL", "B");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");
    RecordProperty("Description", "Verifies that too many arguments shall lead to a truncated log.");
    RecordProperty("TestType", "Interface test");

    constexpr std::size_t kTypeInfoByteSizeAccordingToSpecification = 4;
    const std::size_t number_of_arguments = log_record_.getLogEntry().payload.capacity() /
                                            (kTypeInfoByteSizeAccordingToSpecification + sizeof(std::uint32_t));
    for (std::size_t i = 0; i < number_of_arguments + 5; ++i)
    {
        recorder_->Log(SlotHandle{}, std::uint32_t{});
    }
    EXPECT_LE(number_of_arguments, std::numeric_limits<decltype(expected_number_of_arguments_at_teardown_)>::max());
    expected_number_of_arguments_at_teardown_ =
        static_cast<decltype(expected_number_of_arguments_at_teardown_)>(number_of_arguments);
}

TEST_F(DataRouterRecorderFixture, TooLargeSinglePayloadWillYieldTruncatedLog)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that a too large single payload shall lead to truncate the log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    const std::size_t too_big_data_size = log_record_.getLogEntry().payload.capacity() + 1UL;
    std::vector<char> vec(too_big_data_size);
    std::fill(vec.begin(), vec.end(), 'o');
    recorder_->Log(SlotHandle{}, std::string_view{vec.data(), too_big_data_size});
    recorder_->Log(SlotHandle{}, std::string_view{"xxx"});

    //  Teardown checks if number of arguments is equal to one which means that second argument was ignored due to no
    //  space left in the buffer
}

TEST_F(DataRouterRecorderFixture, LogBool)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording boolean datatype log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, bool{});
}

TEST_F(DataRouterRecorderFixture, LogUint8_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording uint8-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::uint8_t{});
}

TEST_F(DataRouterRecorderFixture, LogInt8_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording int8-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::int8_t{});
}

TEST_F(DataRouterRecorderFixture, LogUint16_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording uint16-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::uint16_t{});
}

TEST_F(DataRouterRecorderFixture, LogInt16_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording int16-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::int16_t{});
}

TEST_F(DataRouterRecorderFixture, LogUint32_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording uint32-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::uint32_t{});
}

TEST_F(DataRouterRecorderFixture, LogInt32_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording int32-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::int32_t{});
}

TEST_F(DataRouterRecorderFixture, LogUint64_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording uint64-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::uint64_t{});
}

TEST_F(DataRouterRecorderFixture, LogInt64_t)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording int64-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::int64_t{});
}

TEST_F(DataRouterRecorderFixture, LogFloat)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording float datatype log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, float{});
}

TEST_F(DataRouterRecorderFixture, LogDouble)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording double datatype log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, double{});
}

TEST_F(DataRouterRecorderFixture, LogStringView)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording stringview datatype log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, std::string_view{"Hello world"});
}

TEST_F(DataRouterRecorderFixture, Log_LogHex8)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording hex8-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogHex8{});
}

TEST_F(DataRouterRecorderFixture, Log_LogHex16)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording hex16-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogHex16{});
}

TEST_F(DataRouterRecorderFixture, Log_LogHex32)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording hex32-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogHex32{});
}

TEST_F(DataRouterRecorderFixture, Log_LogHex64)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording hex64-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogHex64{});
}

TEST_F(DataRouterRecorderFixture, Log_LogBin8)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording binary8-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogBin8{});
}

TEST_F(DataRouterRecorderFixture, Log_LogBin16)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording binary16-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogBin16{});
}

TEST_F(DataRouterRecorderFixture, Log_LogBin32)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording binary32-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogBin32{});
}

TEST_F(DataRouterRecorderFixture, Log_LogBin64)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording binary64-based log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogBin64{});
}

TEST_F(DataRouterRecorderFixture, Log_LogRawBuffer)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording raw buffer log.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogRawBuffer{"raw", 3});
}

TEST_F(DataRouterRecorderFixture, Log_LogSlog2Message)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of recording LogSlog2Message.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    recorder_->Log(SlotHandle{}, LogSlog2Message{11, "slog message"});
}

TEST(DataRouterRecorderTests, DatarouterRecorderShouldClearSlotOnStart)
{
    RecordProperty("Requirement", "SCR-1633236");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Recorder should clean slots before reuse.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Test setup is here since we cannot reuse the fixture here because we have a specific construction use case.
    Configuration config{};
    config.SetDefaultLogLevel(kActiveLogLevel);
    auto backend = std::make_unique<NiceMock<BackendMock>>();
    ON_CALL(*backend, ReserveSlot()).WillByDefault(Return(SlotHandle{}));
    LogRecord log_record{};
    ON_CALL(*backend, GetLogRecord(testing::_)).WillByDefault(ReturnRef(log_record));
    auto recorder = std::make_unique<DataRouterRecorder>(std::move(backend), config);

    // Simulate the case that a slot already contains data from a previous message.
    constexpr auto kContext = "ctx0";
    recorder->StartRecord(kContext, kActiveLogLevel);
    const auto payload = std::string_view{"Hello world"};
    recorder->Log(SlotHandle{}, payload);
    recorder->StopRecord(SlotHandle{});

    // Expect that the previous data is cleared.
    recorder->StartRecord(kContext, kActiveLogLevel);
    EXPECT_EQ(log_record.getVerbosePayload().GetSpan().size(), 0);
    EXPECT_EQ(log_record.getLogEntry().num_of_args, 0);
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
