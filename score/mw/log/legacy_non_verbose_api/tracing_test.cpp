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
#include "score/mw/log/configuration/nvconfigfactory.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"
#include "score/mw/log/legacy_non_verbose_api/tracing.h"

#include "score/datarouter/dlt_filetransfer_trigger_lib/filetransfer_message_trace.h"

#include "static_reflection_with_serialization/serialization/for_logging.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <optional>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
namespace test_data
{
// Test-specific type to avoid singleton contamination
struct DisabledLoggerTestEntry
{
    bool dummy;
};

STRUCT_TRACEABLE(DisabledLoggerTestEntry, dummy)
}  // namespace test_data

namespace
{

using SerializeNs = ::score::common::visitor::logging_serializer;
const std::string kErrorContent1Path =
    "score/mw/log/legacy_non_verbose_api/test/error-content-json-class-id.json";
const std::string kJsonPath = "score/mw/log/legacy_non_verbose_api/test/test-class-id.json";

// Test-specific type for dropped logs counter test
struct DropCounterTestEntry
{
    int test_value;
};

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
        auto k_buffer_size = size;
        buffer1_.resize(k_buffer_size);
        buffer2_.resize(k_buffer_size);
        shared_data_.control_block.control_block_even.data = {buffer1_.data(), k_buffer_size};
        shared_data_.control_block.control_block_odd.data = {buffer2_.data(), k_buffer_size};
        shared_data_.control_block.switch_count_points_active_for_writing = std::uint32_t{1};

        AlternatingReadOnlyReader read_only_reader{shared_data_.control_block,
                                                   shared_data_.control_block.control_block_even.data,
                                                   shared_data_.control_block.control_block_odd.data};
        reader_ = std::make_unique<SharedMemoryReader>(shared_data_, std::move(read_only_reader), []() noexcept {});

        SharedMemoryWriter writer{shared_data_, []() noexcept {}};
        const std::string_view k_ctx{"STDA"};
        const ContextLogLevelMap context_log_level_map{{LoggingIdentifier{k_ctx}, LogLevel::kFatal}};
        config.SetContextLogLevel(context_log_level_map);
        logger = std::make_unique<score::platform::Logger>(config, nv_config, std::move(writer));
        ::score::platform::Logger::InjectTestInstance(logger.get());
    }

    void PrepareContextLogLevelFixture(score::mw::log::NvConfig nv_config, const std::string_view ctxid)
    {
        SharedMemoryWriter writer{shared_data_, []() noexcept {}};
        const ContextLogLevelMap context_log_level_map{{LoggingIdentifier{ctxid}, LogLevel::kError}};
        config.SetContextLogLevel(context_log_level_map);
        logger = std::make_unique<score::platform::Logger>(config, nv_config, std::move(writer));
        ::score::platform::Logger::InjectTestInstance(logger.get());
    }

    void TearDown() override
    {
        ::score::platform::Logger::InjectTestInstance(nullptr);
    }
    void SimulateLogging(const LogLevel log_level = LogLevel::kError,
                         const std::string& context_id = "xxxx",
                         const std::string& app_id = "xxxx")
    {

        const auto slot = unit_.ReserveSlot().value();

        auto&& log_record = unit_.GetLogRecord(slot);
        auto& log_entry = log_record.getLogEntry();

        log_entry.app_id = LoggingIdentifier{app_id};
        log_entry.ctx_id = LoggingIdentifier{context_id};
        log_entry.log_level = log_level;
        log_entry.num_of_args = 5;
        log_record.getVerbosePayload().Put("xyz xyz", 7);

        unit_.FlushSlot(slot);

        const auto acquire_result = logger->GetSharedMemoryWriter().ReadAcquire();
        config = logger->GetConfig();

        reader_->NotifyAcquisitionSetReader(acquire_result);

        reader_->Read([](const TypeRegistration&) noexcept {},
                      [this](const SharedMemoryRecord& record) {
                          std::ignore = SerializeNs::deserialize(
                              record.payload.data(), GetDataSizeAsLength(record.payload), header);
                      });
    }

    Configuration config{};
    std::unique_ptr<score::platform::Logger> logger{};
    LogEntry header{};

  private:
    SharedData shared_data_{};
    std::unique_ptr<SharedMemoryReader> reader_{};
    DatarouterMessageClientStubFactory message_client_factory_{};

    std::vector<Byte> buffer1_{};
    std::vector<Byte> buffer2_{};
    DataRouterBackend unit_{std::uint8_t{255UL}, LogRecord{}, message_client_factory_, config, WriterFactory{{}}};
};

TEST_F(LoggerFixture, WhenCreatingSharedMemoryWriterwithNotEnoughBufferSizeRegesteringNewTypeShallFail)
{
    RecordProperty("Requirement", "SCR-861827,SCR-1633921,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "When Creating Shared memory writer with not enough buffer size, registering new type shall fail");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    PrepareFixture(NvConfigFactory::CreateEmpty(), 1);
    SimulateLogging();
}

TEST_F(LoggerFixture, WhenCreatingSharedMemoryWriterwithOneKiloBytesBufferSizeRegesteringNewTypeShallFail)
{
    RecordProperty("Requirement", "SCR-861827,SCR-1633921,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "When Creating Shared memory writer with not enough buffer size (1 kiloBytes), registering new type "
                   "shall fail");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 1024UL;
    PrepareFixture(NvConfigFactory::CreateEmpty(), kBufferSize);
    SimulateLogging();
}

TEST_F(LoggerFixture, WhenProvidingCorrectNvConfigGetTypeLevelAndThreshold)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The log message shall be disabled if the log level is above to the threshold.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    auto nv_config_result = NvConfigFactory::CreateAndInit(kJsonPath);
    ASSERT_TRUE(nv_config_result.has_value());
    PrepareFixture(std::move(nv_config_result.value()));
    EXPECT_EQ(score::platform::LogLevel::kError, logger->GetTypeLevel<score::mw::log::detail::LogEntry>());
    EXPECT_EQ(score::platform::LogLevel::kFatal, logger->GetTypeThreshold<score::mw::log::detail::LogEntry>());
}

TEST_F(LoggerFixture, WhenProvidingNvConfigWithErrorShallGetErrorContent)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-7263547,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Unable to parse the JSON file due to error in the content.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    PrepareFixture(NvConfigFactory::CreateEmpty());
}

TEST(LoggerFallback, WhenProperWriterNotProvidedFailSafeFallbackShallBeReturned)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verify the ability of returning failsafe fallback in case of a wrong writer was provided.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    score::cpp::optional<SharedMemoryWriter> writer{score::cpp::nullopt};
    Configuration config{};
    auto nv_config = NvConfigFactory::CreateEmpty();
    std::unique_ptr<score::platform::Logger> logger{};
    logger = std::make_unique<score::platform::Logger>(config, nv_config, std::move(writer));

    const auto acquire_result = logger->GetSharedMemoryWriter().ReadAcquire();
    EXPECT_EQ(acquire_result.acquired_buffer, 1UL);
}

TEST(LoggerFallback, AllArgsNulloptShallReturnFailsafeFallback)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verify the ability of returning failsafe fallback in case of initialize logger with no arguments.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    std::unique_ptr<score::platform::Logger> logger{};
    logger = std::make_unique<score::platform::Logger>(score::cpp::nullopt, std::nullopt, score::cpp::nullopt);

    const auto acquire_result = logger->GetSharedMemoryWriter().ReadAcquire();
    EXPECT_EQ(acquire_result.acquired_buffer, 1UL);

    constexpr Length kSmallRequest{1UL};

    //  AllocAndWrite shall discard operation by providing empty span:
    logger->GetSharedMemoryWriter().AllocAndWrite(
        [](const auto data_span) noexcept {
            EXPECT_EQ(data_span.size(), 0);
            return 0UL;
        },
        TypeIdentifier{1UL},
        kSmallRequest);
}

TEST_F(LoggerFixture, WhenProvidingWrongCtxIdWillLeadToVerboseLogLevelThreshold)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The log level should set to verbose when providing wrong ctx id.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    PrepareContextLogLevelFixture(NvConfigFactory::CreateEmpty(), "not supported ctx id");
    EXPECT_EQ(score::platform::LogLevel::kVerbose, logger->GetTypeThreshold<score::mw::log::detail::LogEntry>());
}

TEST_F(LoggerFixture, GetSharedMemoryWriterShallFailWhenThereIsNoSharedMemoryAllocatedUsingLoggerInstanceInitialization)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "The code should terminate when getting the shared memory writer when there is no shared memory allocated. ");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    score::cpp::optional<score::mw::log::detail::SharedMemoryWriter> shared_memory{};
    logger = std::make_unique<score::platform::Logger>(config, NvConfigFactory::CreateEmpty(), std::move(shared_memory));

    const auto acquire_result = logger->GetSharedMemoryWriter().ReadAcquire();
    EXPECT_EQ(acquire_result.acquired_buffer, 1UL);
}

TEST_F(
    LoggerFixture,
    GetSharedMemoryWriterShallFailWhenThereIsNoSharedMemoryAllocatedUsingLoggerInstanceInitializationAndCallingRegisterType)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "The code should terminate when getting the shared-memory writer if there is no shared memory "
                   "allocated during the RegisterType call.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    score::cpp::optional<score::mw::log::detail::SharedMemoryWriter> shared_memory;
    logger = std::make_unique<score::platform::Logger>(config, NvConfigFactory::CreateEmpty(), std::move(shared_memory));
    logger->RegisterType<score::logging::FileTransferEntry>();

    const auto acquire_result = logger->GetSharedMemoryWriter().ReadAcquire();
    EXPECT_EQ(acquire_result.acquired_buffer, 1UL);
}

TEST_F(LoggerFixture, WhenTraceWithLoggerIsNotEnabled)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The testcase verifies logentry enable() as false case.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Create a config with our test type that has ctxid "STDA" and loglevel kError
    std::unordered_map<std::string, score::mw::log::config::NvMsgDescriptor> test_map;
    test_map.emplace("score::mw::log::detail::test_data::DisabledLoggerTestEntry",
                     score::mw::log::config::NvMsgDescriptor(
                         999,
                         score::mw::log::detail::LoggingIdentifier{"TEST"},
                         score::mw::log::detail::LoggingIdentifier{"STDA"},  // Match the context in PrepareFixture
                         score::mw::log::LogLevel::kError));                 // loglevel kError (2)

    score::mw::log::NvConfig test_config{std::move(test_map)};
    PrepareFixture(std::move(test_config));

    // PrepareFixture sets STDA context to kFatal (1)
    // Type loglevel is kError (2)
    // Since threshold (1) < level (2), logger should be disabled
    auto& logger_instance = GetLogEntry<test_data::DisabledLoggerTestEntry>();
    EXPECT_EQ(false, logger_instance.Enabled());
}

struct NonVerboseMessage
{
    bool bool_value;
};

STRUCT_TRACEABLE(NonVerboseMessage, bool_value)
STRUCT_TRACEABLE(DropCounterTestEntry, test_value)

TEST(TraceFixtureTest, WhenTraceWithLogEnabledAndTraceLevelDoesNotExceed)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "This testcase verifies that we can enable log entry with kVerbose(valid) log level");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    NonVerboseMessage entry{false};
    auto& logger = GetLogEntry<NonVerboseMessage>();
    score::platform::LogLevel level = static_cast<score::platform::LogLevel>(0x06U);
    TRACE(entry);
    EXPECT_EQ(true, logger.Enabled());
    EXPECT_EQ(true, logger.EnabledAt(level));
}

TEST(TraceFixtureTest, WhenTraceWithLogLevelEnabledButLevelExceeded)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "This testcase verifies that we can't enabled logentry with invalid log level");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    LogEntry entry{};
    auto& logger = GetLogEntry<score::mw::log::detail::LogEntry>();
    score::platform::LogLevel level = static_cast<score::platform::LogLevel>(0x08U);
    TraceLevel(level, entry);
    EXPECT_EQ(false, logger.EnabledAt(level));
}

TEST_F(LoggerFixture, WhenTypeRegistrationFailsDroppedLogsCounterIsIncremented)
{
    RecordProperty("Description",
                   "When type registration fails, dropped logs counter is incremented for each failed log attempt");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Create a logger with insufficient buffer size to cause registration failures
    constexpr uint64_t kInsufficientBufferSizeToForceRegistrationFailure = 1UL;
    PrepareFixture(NvConfigFactory::CreateEmpty(), kInsufficientBufferSizeToForceRegistrationFailure);

    auto& log_entry_instance = GetLogEntry<DropCounterTestEntry>();

    // Initial counter should be 0
    EXPECT_EQ(0U, log_entry_instance.GetDroppedLogsCount());

    // Attempt to log several times with failed registration
    constexpr int kNumberOfLogAttempts = 5;
    for (int i = 0; i < kNumberOfLogAttempts; ++i)
    {
        DropCounterTestEntry entry{i};
        TRACE(entry);
    }

    // Verify that the counter has been incremented for each failed attempt
    EXPECT_EQ(static_cast<std::uint64_t>(kNumberOfLogAttempts), log_entry_instance.GetDroppedLogsCount());
}

TEST_F(LoggerFixture, WhenContextIdNotInContextLogLevelMapGetTypeThresholdReturnsVerbose)
{
    RecordProperty("Requirement", "SCR-1633147,SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "When a type is registered in nvconfig but its context ID is not in the context log level map, "
                   "get_type_threshold shall return kVerbose (default)");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // Create nvconfig with "score::mw::log::detail::LogEntry" type that has ctxId "STDA"
    auto nv_config_result = NvConfigFactory::CreateAndInit(kJsonPath);
    ASSERT_TRUE(nv_config_result.has_value());

    // Configure context_log_level_map with a DIFFERENT context ID (not "STDA")
    // This creates a scenario where the type exists in nvconfig but its ctxId is not configured
    PrepareContextLogLevelFixture(nv_config_result.value(), "DIFF");

    const auto threshold = logger->GetTypeThreshold<score::mw::log::detail::LogEntry>();

    // Should return kVerbose since ctxId "STDA" is not in the map
    EXPECT_EQ(score::platform::LogLevel::kVerbose, threshold);
}

TEST(TraceFixtureTest, TraceFatalFunctionCallsTraceLevel)
{
    RecordProperty("Requirement", "SCR-1633147");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that TRACE_FATAL calls TRACE_LEVEL with kFatal level");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    LogEntry entry{};
    auto& logger = GetLogEntry<score::mw::log::detail::LogEntry>();
    TraceFatal(entry);
    EXPECT_EQ(true, logger.EnabledAt(score::platform::LogLevel::kFatal));
}

TEST(TraceFixtureTest, TraceWarnFunctionCallsTraceLevel)
{
    RecordProperty("Requirement", "SCR-1633147");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that TRACE_WARN calls TRACE_LEVEL with kWarn level");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    LogEntry entry{};
    auto& logger = GetLogEntry<score::mw::log::detail::LogEntry>();
    TraceWarn(entry);
    EXPECT_EQ(true, logger.EnabledAt(score::platform::LogLevel::kWarn));
}

TEST(TraceFixtureTest, TraceVerboseFunctionCallsTraceLevel)
{
    RecordProperty("Requirement", "SCR-1633147");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that TRACE_VERBOSE calls TRACE_LEVEL with kVerbose level");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    LogEntry entry{};
    auto& logger = GetLogEntry<score::mw::log::detail::LogEntry>();
    TraceVerbose(entry);
    EXPECT_EQ(true, logger.EnabledAt(score::platform::LogLevel::kVerbose));
}

TEST(TraceFixtureTest, TraceDebugFunctionCallsTraceLevel)
{
    RecordProperty("Requirement", "SCR-1633147");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that TRACE_DEBUG calls TRACE_LEVEL with kDebug level");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    LogEntry entry{};
    auto& logger = GetLogEntry<score::mw::log::detail::LogEntry>();
    TraceDebug(entry);
    EXPECT_EQ(true, logger.EnabledAt(score::platform::LogLevel::kDebug));
}

TEST(TraceFixtureTest, TraceInfoFunctionCallsTraceLevel)
{
    RecordProperty("Requirement", "SCR-1633147");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that TRACE_INFO calls TRACE_LEVEL with kInfo level");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    LogEntry entry{};
    auto& logger = GetLogEntry<score::mw::log::detail::LogEntry>();
    TraceInfo(entry);
    EXPECT_EQ(true, logger.EnabledAt(score::platform::LogLevel::kInfo));
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
