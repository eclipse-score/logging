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

#include "score/mw/log/detail/data_router/data_router_backend.h"

#include "score/os/mocklib/stat_mock.h"
#include "score/os/mocklib/stdlib_mock.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"
#include "score/datarouter/include/logger/logger.h"

#include "static_reflection_with_serialization/serialization/for_logging.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

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

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;

using SerializeNs = ::score::common::visitor::logging_serializer;
const std::string ERROR_CONTENT_1_PATH = "score/datarouter/test/ut/data/error-content-json-class-id.json";
const std::string JSON_PATH = "score/datarouter/test/ut/data/test-class-id.json";
struct DatarouterMessageClientStub : DatarouterMessageClient
{
    void Run() override {}
    void Shutdown() override {}
};

class DatarouterMessageClientStubFactory : public DatarouterMessageClientFactory
{
  public:
    std::unique_ptr<DatarouterMessageClient> CreateOnce(const std::string&, const std::string&) override
    {
        return std::make_unique<DatarouterMessageClientStub>();
    }
};

class LoggerFixture : public ::testing::Test
{
  public:
    void PrepareFixture(score::mw::log::NvConfig nv_config, uint64_t size = 1024UL)
    {
        auto kBufferSize = size;
        buffer1_.resize(kBufferSize);
        buffer2_.resize(kBufferSize);
        shared_data_.control_block.control_block_1.data = {buffer1_.data(), static_cast<int64_t>(kBufferSize)};
        shared_data_.control_block.control_block_2.data = {buffer2_.data(), static_cast<int64_t>(kBufferSize)};

        reader_ = std::make_unique<SharedMemoryReader>(shared_data_,
                                                       shared_data_.control_block.control_block_1.data,
                                                       shared_data_.control_block.control_block_2.data,
                                                       []() noexcept {});

        SharedMemoryWriter writer{shared_data_, []() noexcept {}};
        const score::cpp::string_view kCtx{"STDA"};
        const ContextLogLevelMap context_log_level_map{{LoggingIdentifier{kCtx}, LogLevel::kError}};
        config_.SetContextLogLevel(context_log_level_map);
        logger_ = std::make_unique<score::platform::logger>(config_, nv_config, std::move(writer));
        ::score::platform::logger::InjectTestInstance(logger_.get());
    }

    void TearDown() override
    {
        ::score::platform::logger::InjectTestInstance(nullptr);
    }
    void SimulateLogging(const LogLevel logLevel = LogLevel::kError,
                         const std::string& context_id = "xxxx",
                         const std::string& app_id = "xxxx")
    {

        const auto slot = unit_.ReserveSlot().value();

        auto&& logRecord = unit_.GetLogRecord(slot);
        auto& log_entry = logRecord.getLogEntry();

        log_entry.app_id = LoggingIdentifier{app_id};
        log_entry.ctx_id = LoggingIdentifier{context_id};
        log_entry.log_level = static_cast<std::uint8_t>(logLevel);
        log_entry.num_of_args = 5;
        logRecord.getVerbosePayload().Put("xyz xyz", 7);

        unit_.FlushSlot(slot);

        const auto acquire_result = logger_->GetSharedMemoryWriter().ReadAcquire();
        config_ = logger_->get_config();

        reader_->NotifyAcquisition(acquire_result);

        reader_->Read([](const TypeRegistration&) noexcept {},
                      [this](const SharedMemoryRecord& record) {
                          score::cpp::ignore = SerializeNs::deserialize(
                              record.payload.data(), GetDataSizeAsLength(record.payload), header_);
                      });
    }

    Configuration config_{};
    std::unique_ptr<score::platform::logger> logger_{};
    LogEntry header_{};

  private:
    SharedData shared_data_{};
    std::unique_ptr<SharedMemoryReader> reader_{};
    DatarouterMessageClientStubFactory message_client_factory_{};

    std::vector<Byte> buffer1_{};
    std::vector<Byte> buffer2_{};
    DataRouterBackend unit_{std::uint8_t{255UL}, LogRecord{}, message_client_factory_, config_};
};

TEST_F(LoggerFixture, WhenCreatingSharedMemoryWriterwithNotEnoughBufferSizeRegesteringNewTypeShallFail)
{
    RecordProperty("Requirement", "SCR-861827,SCR-1633921,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "When Creating Shared memory writer with not enough buffer size, registering new type shall fail");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    PrepareFixture(score::mw::log::NvConfig{JSON_PATH}, 1);
    SimulateLogging();
}

TEST_F(LoggerFixture, WhenProvidingCorrectNvConfigGetTypeLevelAndThreshold)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The log message shall be disabled if the log level is above to the threshold.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    PrepareFixture(score::mw::log::NvConfig{JSON_PATH});
    EXPECT_EQ(score::platform::LogLevel::kError, logger_->get_type_level<score::mw::log::detail::LogEntry>());
    EXPECT_EQ(score::platform::LogLevel::kError, logger_->get_type_threshold<score::mw::log::detail::LogEntry>());
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
