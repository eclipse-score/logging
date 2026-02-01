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

#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/mman_mock.h"
#include "score/os/mocklib/stat_mock.h"
#include "score/os/mocklib/stdlib_mock.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/mw/log/configuration/nvconfigfactory.h"
#include "score/mw/log/detail/data_router/data_router_message_client_factory_mock.h"
#include "score/mw/log/detail/data_router/data_router_message_client_mock.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"
#include "score/mw/log/legacy_non_verbose_api/tracing.h"

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
using ::testing::Return;
using ::testing::StrEq;

constexpr pid_t kPid{0x314};
constexpr std::int32_t kArbitratyUid = 21880012;
constexpr std::int32_t kFileDescriptor = 0x31;  //  arbitrary file descriptor number
constexpr std::int32_t kFdNumber = 17;
constexpr auto kSharedSize = 64UL;
constexpr auto kOpenReadFlagsDynamic =
    score::os::Fcntl::Open::kReadWrite | score::os::Fcntl::Open::kExclusive | score::os::Fcntl::Open::kCloseOnExec;
constexpr auto kOpenReadFlags = kOpenReadFlagsDynamic | score::os::Fcntl::Open::kCreate;
constexpr auto kOpenModeFlags =
    score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kReadGroup | score::os::Stat::Mode::kReadOthers;
constexpr auto kAlignRequirement = std::alignment_of<SharedData>::value;
const char kDynamicFileName[] = "/tmp/logging-XXXXXX.shmem";

std::string GetStaticSharedMemoryFileName() noexcept
{
    std::stringstream ss;
    ss << "/tmp/logging.NONE." << kArbitratyUid << ".shmem";
    return ss.str();
}

using SerializeNs = ::score::common::visitor::logging_serializer;

struct DatarouterMessageClientStub : DatarouterMessageClient
{
    void Run() override {}
    void Shutdown() override {}
};

WriterFactory::OsalInstances CreateSharedMemoryWriterFactoryMockResources()
{
    WriterFactory::OsalInstances osal;
    auto* memory_resource = score::cpp::pmr::get_default_resource();
    osal.fcntl_osal = score::cpp::pmr::make_unique<score::os::FcntlMock>(memory_resource);
    osal.unistd = score::cpp::pmr::make_unique<score::os::UnistdMock>(memory_resource);
    osal.mman = score::cpp::pmr::make_unique<score::os::MmanMock>(memory_resource);
    osal.stat_osal = score::cpp::pmr::make_unique<score::os::StatMock>(memory_resource);
    osal.stdlib = score::cpp::pmr::make_unique<score::os::StdlibMock>(memory_resource);

    return osal;
}

class DatarouterMessageClientStubFactory : public DatarouterMessageClientFactory
{
  public:
    std::unique_ptr<DatarouterMessageClient> CreateOnce(const std::string&, const std::string&) override
    {
        return std::make_unique<DatarouterMessageClientStub>();
    }
};

/// The main purpose of this test is to test the connection from our logging library to DataRouter.
///
/// In the first step we will use the available TRACE infrastructure. For that purpose the following tests
/// ensure that the data that is feed into our Backend are correctly transformed and serialized, thus
/// that DataRouter can interpret them.
/// The protocol to DataRouter is implementation specific and described below:
///
/// +--------+----------+--------+--------+--------+--------+-----------+---------+
/// | Byte 0 |  Byte 1  | Byte 2 | Byte 3 | Byte 4 | Byte 5 |  Byte 6   |  Byte 7 |
/// +--------+----------+--------+--------+--------+--------+-----------+---------+
/// |          Format Version             |                APP ID                 |
/// +-------------------------------------+-----------------+-----------+---------+
/// |             CTX ID                  | Number of Args  | Log Level | payload |
/// +-------------------------------------+-----------------+-----------+---------+
/// |                                  payload                                    |
/// +-----------------------------------------------------------------------------+

class DataRouterBackendFixture : public ::testing::Test, public ::testing::WithParamInterface<LogLevel>
{
  public:
    DataRouterBackendFixture() : shared_data_{}, writer_(InitializeSharedData(shared_data_), []() noexcept {})
    {
        constexpr auto kBufferSize = UINT64_C(1024);
        buffer1_.resize(kBufferSize);
        buffer2_.resize(kBufferSize);
        shared_data_.control_block.control_block_even.data = {buffer1_.data(), static_cast<int64_t>(kBufferSize)};
        shared_data_.control_block.control_block_odd.data = {buffer2_.data(), static_cast<int64_t>(kBufferSize)};

        AlternatingReadOnlyReader read_only_reader{shared_data_.control_block,
                                                   shared_data_.control_block.control_block_even.data,
                                                   shared_data_.control_block.control_block_odd.data};
        reader_ = std::make_unique<SharedMemoryReader>(shared_data_, std::move(read_only_reader), []() noexcept {});

        config.SetDynamicDatarouterIdentifiers(false);
        logger = std::make_unique<score::platform::logger>(
            config, score::mw::log::NvConfigFactory::CreateEmpty(), std::move(writer_));

        ::score::platform::logger::InjectTestInstance(logger.get());

        CreateSharedMemoryWriterFactory();

        map_address = buffer.data();

        ON_CALL(*stdlib_mock_raw_ptr, mkstemps(_, _)).WillByDefault(Return(kFdNumber));

        ON_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, _))
            .WillByDefault(Return(score::cpp::expected_blank<score::os::Error>{}));

        ON_CALL(*stat_mock_raw_ptr, chmod(_, _)).WillByDefault(Return(score::cpp::expected_blank<score::os::Error>{}));

        ON_CALL(*unistd_mock_raw_ptr, getuid()).WillByDefault(Return(kArbitratyUid));

        ON_CALL(*mman_mock_raw_ptr,
                mmap(nullptr,
                     _,
                     score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                     score::os::Mman::Map::kShared,
                     kFileDescriptor,
                     0))
            .WillByDefault(Return(score::cpp::expected<void*, score::os::Error>{map_address}));

        ON_CALL(*unistd_mock_raw_ptr, getpid()).WillByDefault(Return(kPid));
    }

    void TearDown() override
    {
        ::score::platform::logger::InjectTestInstance(nullptr);
    }

    void SimulateLogging(const std::string& context_id, const LogLevel log_level = LogLevel::kError)
    {
        SimulateLogging(log_level, context_id);
    }

    void CreateSharedMemoryWriterFactory()
    {
        auto* memory_resource = score::cpp::pmr::get_default_resource();
        auto fcntl_mock = score::cpp::pmr::make_unique<score::os::FcntlMock>(memory_resource);
        auto unistd_mock = score::cpp::pmr::make_unique<score::os::UnistdMock>(memory_resource);
        auto mman_mock = score::cpp::pmr::make_unique<score::os::MmanMock>(memory_resource);
        auto stat_mock = score::cpp::pmr::make_unique<score::os::StatMock>(memory_resource);
        auto stdlib_mock = score::cpp::pmr::make_unique<score::os::StdlibMock>(memory_resource);

        fcntl_mock_raw_ptr = fcntl_mock.get();
        mman_mock_raw_ptr = mman_mock.get();
        unistd_mock_raw_ptr = unistd_mock.get();
        stat_mock_raw_ptr = stat_mock.get();
        stdlib_mock_raw_ptr = stdlib_mock.get();

        WriterFactory::OsalInstances osal;
        osal.fcntl_osal = std::move(fcntl_mock);
        osal.unistd = std::move(unistd_mock);
        osal.mman = std::move(mman_mock);
        osal.stat_osal = std::move(stat_mock);
        osal.stdlib = std::move(stdlib_mock);

        writer_factory = WriterFactory{std::move(osal)};
    }

    void SimulateLogging(const LogLevel log_level = LogLevel::kError,
                         const std::string& context_id = "DFLT",
                         const std::string& app_id = "TEST")
    {

        const auto slot = unit_.ReserveSlot().value();

        auto&& log_record = unit_.GetLogRecord(slot);
        auto& log_entry = log_record.getLogEntry();

        log_entry.app_id = LoggingIdentifier{app_id};
        log_entry.ctx_id = LoggingIdentifier{context_id};
        log_entry.log_level = log_level;
        log_entry.num_of_args = 5;
        log_record.getVerbosePayload().Put("ABC DEF", 7);

        unit_.FlushSlot(slot);

        const auto acquire_result = logger->GetSharedMemoryWriter().ReadAcquire();
        config = logger->get_config();

        reader_->NotifyAcquisitionSetReader(acquire_result);

        reader_->Read([](const TypeRegistration&) noexcept {},
                      [this](const SharedMemoryRecord& record) {
                          std::ignore = SerializeNs::deserialize(
                              record.payload.data(), GetDataSizeAsLength(record.payload), header);
                      });
    }

    LogEntry header{};
    std::unique_ptr<score::platform::logger> logger{};
    Configuration config{};

    //  Mocks needed for dependency injection into SharedMemoryWriter:
    score::os::FcntlMock* fcntl_mock_raw_ptr;
    score::os::UnistdMock* unistd_mock_raw_ptr;
    score::os::StatMock* stat_mock_raw_ptr;
    score::os::StdlibMock* stdlib_mock_raw_ptr;
    score::os::MmanMock* mman_mock_raw_ptr;

    score::mw::log::detail::WriterFactory writer_factory{{}};
    std::array<Byte, kSharedSize + kAlignRequirement> buffer;
    void* map_address;

  private:
    SharedData shared_data_{};
    SharedMemoryWriter writer_;
    std::vector<Byte> buffer1_{};
    std::vector<Byte> buffer2_{};

    std::unique_ptr<SharedMemoryReader> reader_{};
    DatarouterMessageClientStubFactory message_client_factory_{};
    score::mw::log::NvConfig nv_config_{score::mw::log::NvConfigFactory::CreateEmpty()};
    DataRouterBackend unit_{std::uint8_t{255UL},
                            LogRecord{},
                            message_client_factory_,
                            config,
                            WriterFactory{CreateSharedMemoryWriterFactoryMockResources()}};
};

TEST_F(DataRouterBackendFixture, AppIDSetCorrectly)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies setting the app id.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    SimulateLogging();

    const auto app_id = LoggingIdentifier{"TEST"};
    EXPECT_EQ(header.app_id, app_id);
}

TEST_F(DataRouterBackendFixture, ToSmallAppIdSetCorrectlyWithZeroPadding)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies setting the app id with zero padding.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    SimulateLogging(LogLevel::kError, "DFLT", "TES");

    const auto test = LoggingIdentifier{"TES"};
    EXPECT_EQ(header.app_id, test);
}

TEST_F(DataRouterBackendFixture, ContextIdSetCorrectly)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies setting the context id.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    SimulateLogging();

    const auto ctx = LoggingIdentifier{"DFLT"};
    EXPECT_EQ(header.ctx_id, ctx);
}

TEST_F(DataRouterBackendFixture, ToSmallCtxSetCorrectlyWithZeroPadding)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies setting the context id with zero padding.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    SimulateLogging("DFL");

    const auto ctx = LoggingIdentifier{"DFL"};
    EXPECT_EQ(header.ctx_id, ctx);
}

TEST_P(DataRouterBackendFixture, LogLevelSetCorrectly)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "DatarouterBackend is internally using the same API as for structured logging. Check that the API "
                   "is used correctly and the log level is propagated.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    SimulateLogging(GetParam());

    EXPECT_EQ(header.log_level, GetParam());
}

INSTANTIATE_TEST_SUITE_P(LogLevelSetCorrectly,
                         DataRouterBackendFixture,
                         ::testing::Values(LogLevel::kFatal,
                                           LogLevel::kError,
                                           LogLevel::kWarn,
                                           LogLevel::kInfo,
                                           LogLevel::kDebug,
                                           LogLevel::kVerbose));

TEST_F(DataRouterBackendFixture, LogLevelVerbose)
{
    RecordProperty("ParentRequirement", "SCR-1633144");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify the ability of logging verbose message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SimulateLogging(LogLevel::kVerbose);
}

TEST_F(DataRouterBackendFixture, LogLevelDebug)
{
    RecordProperty("ParentRequirement", "SCR-1633144");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify the ability of logging debug message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SimulateLogging(LogLevel::kDebug);
}

TEST_F(DataRouterBackendFixture, LogLevelInfo)
{
    RecordProperty("ParentRequirement", "SCR-1633144");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify the ability of logging info message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SimulateLogging(LogLevel::kInfo);
}

TEST_F(DataRouterBackendFixture, LogLevelWarning)
{
    RecordProperty("ParentRequirement", "SCR-1633144");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify the ability of logging warning message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SimulateLogging(LogLevel::kWarn);
}

TEST_F(DataRouterBackendFixture, LogLevelError)
{
    RecordProperty("ParentRequirement", "SCR-1633144 ");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify the ability of logging error message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SimulateLogging(LogLevel::kError);
}

TEST_F(DataRouterBackendFixture, LogLevelFatal)
{
    RecordProperty("ParentRequirement", "SCR-1633144");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify the ability of logging fatal message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SimulateLogging(LogLevel::kFatal);
}

TEST_F(DataRouterBackendFixture, LogLevelOff)
{
    RecordProperty("ParentRequirement", "SCR-1633144");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify the ability of disabling the logging.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SimulateLogging(LogLevel::kOff);
}

TEST_F(DataRouterBackendFixture, NumberOfArgsSetCorrectly)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies setting the number of arguments correctly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    SimulateLogging();

    EXPECT_EQ(header.num_of_args, 5UL);
}

TEST_F(DataRouterBackendFixture, PayloadSetCorrectly)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies setting the payload correctly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    SimulateLogging();

    ByteVector payload{'A', 'B', 'C', ' ', 'D', 'E', 'F'};

    // Payload content
    EXPECT_EQ(header.payload, payload);
}

TEST(DataRouterBackendTests, CheckSizeValid)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of reserving more slots above the maximum slot size limit.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Analysis of boundary values");

    const std::size_t k_max_slots_size = 255UL;
    DatarouterMessageClientStubFactory message_client_factory;
    //  Give the try to allocate one more then possible number of slots
    const Configuration config{};
    WriterFactory writer_factory{CreateSharedMemoryWriterFactoryMockResources()};
    DataRouterBackend datarouter_backend{
        k_max_slots_size + 1, LogRecord{}, message_client_factory, config, std::move(writer_factory)};

    // Given depleted allocator:
    for (std::size_t i = 0; i < k_max_slots_size; i++)
    {
        const auto& slot = datarouter_backend.ReserveSlot();
        EXPECT_TRUE(slot.has_value());
    }

    //  Expect slot allocation to fail
    const auto& slot = datarouter_backend.ReserveSlot();
    EXPECT_FALSE(slot.has_value());
}

TEST(DataRouterBackendTests, WhenAllPossibleSlotsUsedFailToAllocateMore)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of reserving more slots above the maximum slot size limit.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Analysis of boundary values");

    // Given maksimum allocation size:
    const std::uint8_t k_max_slots_size = 255UL;
    DatarouterMessageClientStubFactory message_client_factory{};
    const Configuration config{};
    DataRouterBackend datarouter_backend(k_max_slots_size,
                                         LogRecord{},
                                         message_client_factory,
                                         config,
                                         WriterFactory{CreateSharedMemoryWriterFactoryMockResources()});

    // Given depleted allocator:
    for (std::size_t i = 0; i < k_max_slots_size; i++)
    {
        const auto& slot = datarouter_backend.ReserveSlot();
        EXPECT_TRUE(slot.has_value());
    }

    //  Expect slot allocation to fail
    const auto& slot = datarouter_backend.ReserveSlot();
    EXPECT_FALSE(slot.has_value());
}

TEST_F(DataRouterBackendFixture, WhenSafeIpcIsTrueMessageClientIsCreated)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of creating message client instance.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    const std::uint8_t k_max_slots_size = 255UL;
    DatarouterMessageClientFactoryMock message_client_factory{};

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(GetStaticSharedMemoryFileName()), kOpenReadFlags, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(message_client_factory, CreateOnce(_, _))
        .WillOnce([](const std::string&, const std::string&) -> std::unique_ptr<DatarouterMessageClient> {
            return std::make_unique<DatarouterMessageClientMock>();
        });

    DataRouterBackend datarouter_backend(
        k_max_slots_size, LogRecord{}, message_client_factory, config, std::move(writer_factory));
}

TEST_F(DataRouterBackendFixture, WhenIdentifierIsNotDynamicUidShallBeUsed)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the construction of data router backend without dynamic identifier.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    const std::uint8_t k_slots_size = 16UL;
    DatarouterMessageClientFactoryMock message_client_factory{};

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(GetStaticSharedMemoryFileName()), kOpenReadFlags, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(message_client_factory, CreateOnce(_, _))
        .WillOnce([](const std::string&, const std::string&) -> std::unique_ptr<DatarouterMessageClient> {
            return std::make_unique<DatarouterMessageClientMock>();
        });

    DataRouterBackend datarouter_backend(
        k_slots_size, LogRecord{}, message_client_factory, config, std::move(writer_factory));
}

TEST_F(DataRouterBackendFixture, ConstructWithDynamicIdentifier)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the construction of data router backend with dynamic identifier.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    const std::uint8_t k_max_slots_size = 255UL;
    DatarouterMessageClientFactoryMock message_client_factory{};
    config.SetDynamicDatarouterIdentifiers(true);

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kDynamicFileName), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(message_client_factory, CreateOnce(_, _))
        .WillOnce([](const std::string&, const std::string&) -> std::unique_ptr<DatarouterMessageClient> {
            return std::make_unique<DatarouterMessageClientMock>();
        });

    EXPECT_CALL(*unistd_mock_raw_ptr, getuid()).Times(0);

    DataRouterBackend datarouter_backend(
        k_max_slots_size, LogRecord{}, message_client_factory, config, std::move(writer_factory));
}

TEST_F(DataRouterBackendFixture, ConstructWithDynamicIdentifierAndChmodSuccess)
{
    const std::uint8_t k_max_slots_size = 255UL;
    DatarouterMessageClientFactoryMock message_client_factory{};
    config.SetDynamicDatarouterIdentifiers(true);

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kDynamicFileName), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(message_client_factory, CreateOnce(_, _))
        .WillOnce([](const std::string&, const std::string&) -> std::unique_ptr<DatarouterMessageClient> {
            return std::make_unique<DatarouterMessageClientMock>();
        });

    EXPECT_CALL(*unistd_mock_raw_ptr, getuid()).Times(0);

    DataRouterBackend datarouter_backend(
        k_max_slots_size, LogRecord{}, message_client_factory, config, std::move(writer_factory));
}

TEST_F(DataRouterBackendFixture, DataRouterBackEndConstructedWithEmptyIdentifierWhenMkstempFail)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the construction of data router backend when providing empty identifier.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    config.SetDynamicDatarouterIdentifiers(true);

    const std::uint8_t k_max_slots_size = 255UL;
    DatarouterMessageClientFactoryMock message_client_factory{};

    EXPECT_CALL(*stdlib_mock_raw_ptr, mkstemps(_, _))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));

    EXPECT_CALL(*stat_mock_raw_ptr, chmod(_, _))
        .Times(2)
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));

    EXPECT_CALL(message_client_factory, CreateOnce(_, _)).Times(0);

    EXPECT_CALL(*unistd_mock_raw_ptr, getuid()).Times(0);

    DataRouterBackend datarouter_backend(
        k_max_slots_size, LogRecord{}, message_client_factory, config, std::move(writer_factory));
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
