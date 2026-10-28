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

#include "score/datarouter/datarouter/data_router.h"

#include "logparser/logparser.h"

#include "score/mw/log/detail/data_router/shared_memory/reader_factory_mock.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_writer.h"
#include "score/datarouter/daemon_communication/session_handle_mock.h"
#include "score/os/mocklib/unistdmock.h"

#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/configuration/nvconfigfactory.h"

#include "static_reflection_with_serialization/serialization/for_logging.h"
#include "static_reflection_with_serialization/serialization/visit_size.h"

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader_mock.h"
#include "score/datarouter/mocks/log_parser_factory_mock.h"
#include "score/datarouter/mocks/logparser_mock.h"

#include "gtest/gtest.h"

namespace test::utils
{

namespace
{
struct TestTypeInfo
{
    std::size_t size() const
    {
        return type_params.size();
    }

    void Copy(const score::cpp::span<score::mw::log::detail::Byte> data) const
    {
        if (score::mw::log::detail::GetDataSizeAsLength(data) != type_params.size())
        {
            std::abort();
        }
        std::memcpy(data.data(), type_params.data(), type_params.size());
    }

    std::string type_params{};
};

template <typename Message>
TestTypeInfo CreateTypeInfo()
{
    constexpr static std::size_t kIdsize = score::platform::DltidT::size();
    const std::string app_prefix(kIdsize * 3, char{0});
    TestTypeInfo type_info{};
    type_info.type_params = app_prefix + ::score::common::visitor::logger_type_string<Message>();
    return type_info;
}

}  // namespace
}  // namespace test::utils

using namespace testing;
using namespace test;

namespace score
{
namespace platform
{
namespace datarouter
{

class LoggerMock
{
  public:
    struct LogStream
    {
        template <typename T>
        LogStream& operator<<(const T&)
        {
            return *this;
        }
    };

    LogStream LogInfo()
    {
        return {};
    }
    LogStream LogError()
    {
        return {};
    }
    LogStream LogWarning()
    {
        return {};
    }
};

class DataRouter::DataRouterForTest : public DataRouter
{
  public:
    using DataRouter::DataRouter;
    using DataRouter::SourceSession;
};

class DataRouter::DataRouterForTest::SourceSession::SourceSessionForTest : public DataRouter::SourceSession
{
  public:
    using DataRouter::SourceSession::IsSourceClosed;
    using DataRouter::SourceSession::OnAcquireResponse;
    using DataRouter::SourceSession::ProcessAndRouteLogMessages;
    using DataRouter::SourceSession::ProcessDetachedLogs;
    using DataRouter::SourceSession::RequestAcquire;
    using DataRouter::SourceSession::SourceSession;
    using DataRouter::SourceSession::TryFinalizeAcquisition;

    using DataRouter::SourceSession::command_data_;
    using DataRouter::SourceSession::handle_;
    using DataRouter::SourceSession::local_subscriber_data_;
    using DataRouter::SourceSession::stats_data_;

    SourceSessionForTest(DataRouter& router,
                         std::unique_ptr<score::mw::log::detail::ISharedMemoryReader> reader,
                         const std::string& name,
                         const bool is_dlt_enabled,
                         SessionHandleVariant handle,
                         const double quota,
                         bool quota_enforcement_enabled,
                         score::mw::log::Logger& stats_logger,
                         std::unique_ptr<score::platform::internal::ILogParser> parser)
        : SourceSession(router,
                        std::move(reader),
                        name,
                        is_dlt_enabled,
                        std::move(handle),
                        quota,
                        quota_enforcement_enabled,
                        stats_logger,
                        std::move(parser))
    {
    }
};

using namespace score::platform;
using namespace score::platform::internal;

class SourceSessionTestFixture : public ::testing::Test
{
  public:
    SourceSessionTestFixture() : logger_(score::mw::log::CreateLogger("DRUT", "statistics")), router_(logger_) {}

  public:
    void SetUp() override
    {
        auto shared_memory_reader_mock = std::make_unique<score::mw::log::detail::ISharedMemoryReaderMock>();
        shared_memory_reader_mock_ = shared_memory_reader_mock.get();
        auto log_parser_mock = std::make_unique<internal::LogParserMock>();
        log_parser_mock_ = log_parser_mock.get();
        auto session_handle_mock_raw =
            score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
        session_handle_mock_ = session_handle_mock_raw.get();

        // Set default return values for uninteresting mock calls to suppress warnings
        ON_CALL(*session_handle_mock_, AcquireRequest()).WillByDefault(Return(false));

        // Move-convert to base interface type
        score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> session_handle_mock =
            std::move(session_handle_mock_raw);

        source_session_under_test_ =
            std::make_unique<DataRouter::DataRouterForTest::SourceSession::SourceSessionForTest>(
                router_,
                std::move(shared_memory_reader_mock),
                "source1",
                false,
                std::move(session_handle_mock),
                0,
                false,
                logger_,
                std::move(log_parser_mock));
    }

  protected:
    score::mw::log::Logger& logger_;
    datarouter::DataRouter router_;
    score::mw::log::detail::ISharedMemoryReaderMock* shared_memory_reader_mock_;
    internal::LogParserMock* log_parser_mock_;
    daemon::mock::SessionHandleMock* session_handle_mock_;

    std::unique_ptr<DataRouter::DataRouterForTest::SourceSession::SourceSessionForTest> source_session_under_test_;
};

TEST_F(SourceSessionTestFixture, TryFinalizeAcquisitionIsBlockReleased)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that acquisition is finalized once the writers released the block.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    bool needs_fast_reschedule = true;

    EXPECT_CALL(*shared_memory_reader_mock_, IsBlockReleasedByWriters(_)).WillOnce(Return(true));

    // any value except null
    source_session_under_test_->command_data_.lock()->data_acquired = score::mw::log::detail::ReadAcquireResult();

    EXPECT_TRUE(source_session_under_test_->TryFinalizeAcquisition(needs_fast_reschedule));
}

TEST_F(SourceSessionTestFixture, TryFinalizeAcquisitionIsBlockNotReleased)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that acquisition is not finalized while the writers still hold the block.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    bool needs_fast_reschedule = true;

    EXPECT_CALL(*shared_memory_reader_mock_, IsBlockReleasedByWriters(_)).WillOnce(Return(false));

    // any value except null
    source_session_under_test_->command_data_.lock()->data_acquired = score::mw::log::detail::ReadAcquireResult();

    EXPECT_FALSE(source_session_under_test_->TryFinalizeAcquisition(needs_fast_reschedule));
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesAcquireFinalized)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a finalized acquisition resets the acquire request state.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = true;
    bool needs_fast_reschedule = false;

    source_session_under_test_->ProcessAndRouteLogMessages(message_count_local,
                                                           transport_delay_local,
                                                           number_of_bytes_in_buffer,
                                                           acquire_finalized_in_this_tick,
                                                           needs_fast_reschedule);

    ASSERT_TRUE(source_session_under_test_->command_data_.lock());

    EXPECT_FALSE(source_session_under_test_->command_data_.lock()->acquire_requested);
    EXPECT_EQ(source_session_under_test_->command_data_.lock()->ticks_without_write, 0);
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesBlockExpectedToBeNextDataAvailable)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a new acquire is requested when the expected block has data.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    source_session_under_test_->command_data_.lock()->acquire_requested = false;
    source_session_under_test_->command_data_.lock()->command_detach_on_closed = false;
    source_session_under_test_->command_data_.lock()->block_expected_to_be_next = 0;
    source_session_under_test_->local_subscriber_data_.lock()->enabled_logging_at_server = true;

    EXPECT_CALL(*shared_memory_reader_mock_, PeekNumberOfBytesAcquiredInBuffer(_)).WillOnce(Return(10));
    EXPECT_CALL(*session_handle_mock_, AcquireRequest()).WillOnce(Return(true));

    source_session_under_test_->ProcessAndRouteLogMessages(message_count_local,
                                                           transport_delay_local,
                                                           number_of_bytes_in_buffer,
                                                           acquire_finalized_in_this_tick,
                                                           needs_fast_reschedule);

    EXPECT_TRUE(source_session_under_test_->command_data_.lock()->acquire_requested);
    EXPECT_TRUE(needs_fast_reschedule);
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesBlockExpectedToBeNextDataNotAvailable)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that idle ticks are counted when the expected block has no data.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    const uint8_t ticks_without_write = 1;

    source_session_under_test_->command_data_.lock()->acquire_requested = false;
    source_session_under_test_->command_data_.lock()->command_detach_on_closed = false;
    source_session_under_test_->command_data_.lock()->block_expected_to_be_next = 0;
    source_session_under_test_->command_data_.lock()->ticks_without_write = ticks_without_write;
    source_session_under_test_->local_subscriber_data_.lock()->enabled_logging_at_server = true;

    EXPECT_CALL(*shared_memory_reader_mock_, PeekNumberOfBytesAcquiredInBuffer(_)).WillOnce(Return(std::nullopt));

    source_session_under_test_->ProcessAndRouteLogMessages(message_count_local,
                                                           transport_delay_local,
                                                           number_of_bytes_in_buffer,
                                                           acquire_finalized_in_this_tick,
                                                           needs_fast_reschedule);

    EXPECT_EQ(source_session_under_test_->command_data_.lock()->ticks_without_write, ticks_without_write + 1);
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesNotBlockExpectedToBeNext)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a new acquire is requested when no block is expected next.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    source_session_under_test_->command_data_.lock()->acquire_requested = false;
    source_session_under_test_->command_data_.lock()->command_detach_on_closed = false;
    source_session_under_test_->command_data_.lock()->block_expected_to_be_next = std::nullopt;
    source_session_under_test_->local_subscriber_data_.lock()->enabled_logging_at_server = true;

    EXPECT_CALL(*session_handle_mock_, AcquireRequest()).WillOnce(Return(true));

    source_session_under_test_->ProcessAndRouteLogMessages(message_count_local,
                                                           transport_delay_local,
                                                           number_of_bytes_in_buffer,
                                                           acquire_finalized_in_this_tick,
                                                           needs_fast_reschedule);

    EXPECT_TRUE(source_session_under_test_->command_data_.lock()->acquire_requested);
    EXPECT_TRUE(needs_fast_reschedule);
}

TEST_F(SourceSessionTestFixture, RequestAcquire)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that an acquire request can be sent via a Unix domain session handle.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    UnixDomainServer::SessionHandle session_handle(0, nullptr);

    source_session_under_test_->handle_ = session_handle;

    // verify that DataRouter::SourceSession is able
    // to process UnixDomainServer::SessionHandle type
    EXPECT_NO_THROW(source_session_under_test_->RequestAcquire());
}

TEST_F(SourceSessionTestFixture, OnAcquireResponse)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that the acquired buffer of an acquire response is stored.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    source_session_under_test_->command_data_.lock()->data_acquired = std::nullopt;

    const score::mw::log::detail::ReadAcquireResult read_acquire_result{44};
    source_session_under_test_->OnAcquireResponse(read_acquire_result);

    ASSERT_TRUE(source_session_under_test_->command_data_.lock()->data_acquired);
    EXPECT_EQ(source_session_under_test_->command_data_.lock()->data_acquired->acquired_buffer,
              read_acquire_result.acquired_buffer);
}

TEST_F(SourceSessionTestFixture, IsSourceClosed)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a source is reported closed only after its detach was processed.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    // Initially, source should not be closed
    EXPECT_FALSE(source_session_under_test_->IsSourceClosed());

    // Set detach_on_closed_processed to true
    source_session_under_test_->local_subscriber_data_.lock()->detach_on_closed_processed = true;

    // Now source should be closed
    EXPECT_TRUE(source_session_under_test_->IsSourceClosed());
}

// custom action because InvokeArgument could not work with reference to move-only argument
ACTION(InvokeSharedReadArguments)
{
    arg0(score::mw::log::detail::TypeRegistration());
    arg1(score::mw::log::detail::SharedMemoryRecord());
}

TEST_F(SourceSessionTestFixture, ProcessDetachedLogsReaderUsed)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that logs of a detached writer are read and passed to the log parser.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    score::mw::log::detail::TypeRegistration tr;

    EXPECT_CALL(*shared_memory_reader_mock_, ReadDetached(_, _))
        .WillOnce(DoAll(InvokeSharedReadArguments(), Return(std::nullopt)));

    // to be called as side effect from SharedMemoryReader::ReadDetached
    EXPECT_CALL(*log_parser_mock_, AddIncomingType(_));
    EXPECT_CALL(*log_parser_mock_, ParseSharedMemoryRecord(_));

    uint64_t number_of_bytes_in_buffer = 0;
    source_session_under_test_->ProcessDetachedLogs(number_of_bytes_in_buffer);
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesQuotaLimitExceeded)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_quota_bw_reduction");
    RecordProperty("Description", "Check that messages are not parsed while the quota limit is exceeded.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    source_session_under_test_->command_data_.lock()->acquire_requested = false;
    source_session_under_test_->command_data_.lock()->command_detach_on_closed = false;
    source_session_under_test_->command_data_.lock()->block_expected_to_be_next = std::nullopt;
    source_session_under_test_->local_subscriber_data_.lock()->enabled_logging_at_server = true;

    source_session_under_test_->stats_data_.lock()->quota_overlimit_detected = true;

    EXPECT_CALL(*shared_memory_reader_mock_, Read(_, _))
        .WillOnce(DoAll(InvokeSharedReadArguments(), Return(std::nullopt)));

    // to be called as side effect from SharedMemoryReader::Read
    // with quota_overlimit_detected - Parse call to be skipped
    EXPECT_CALL(*log_parser_mock_, AddIncomingType(_));
    EXPECT_CALL(*log_parser_mock_, ParseSharedMemoryRecord(_)).Times(0);

    source_session_under_test_->ProcessAndRouteLogMessages(message_count_local,
                                                           transport_delay_local,
                                                           number_of_bytes_in_buffer,
                                                           acquire_finalized_in_this_tick,
                                                           needs_fast_reschedule);
}

constexpr bool kDefaultDltEnable{true};
class DataRouterTestFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        score::os::Unistd::set_testing_instance(unistd_mock);

        constexpr auto kBufferSize = UINT64_C(1024);
        buffer1.resize(kBufferSize);
        buffer2.resize(kBufferSize);
        shared_data.control_block.control_block_even.data = {buffer1.data(), static_cast<int64_t>(kBufferSize)};
        shared_data.control_block.control_block_odd.data = {buffer2.data(), static_cast<int64_t>(kBufferSize)};
    }

    void TearDown() override
    {
        score::os::Unistd::restore_instance();
    }

    score::mw::log::NvConfig nv_config = score::mw::log::NvConfigFactory::CreateEmpty();

    score::mw::log::detail::ReaderFactoryPtr CreateReaderFactory()
    {
        auto factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
        EXPECT_CALL(*factory, Create(_, _)).WillOnce([this](auto, auto) -> auto {
            return std::make_unique<score::mw::log::detail::SharedMemoryReader>(
                shared_data,
                score::mw::log::detail::AlternatingReadOnlyReader{shared_data.control_block, buffer1, buffer2},
                []() noexcept {});
        });
        return factory;
    }

    score::mw::log::detail::ReaderFactoryPtr CreateReaderFactoryWithDetachedReader()
    {
        auto factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
        EXPECT_CALL(*factory, Create(_, _)).WillOnce([this](auto, auto) -> auto {
            auto reader = std::make_unique<score::mw::log::detail::SharedMemoryReader>(
                shared_data,
                score::mw::log::detail::AlternatingReadOnlyReader{shared_data.control_block, buffer1, buffer2},
                []() noexcept {});
            // detach reader to simulate read error on next read
            reader->ReadDetached([](const score::mw::log::detail::TypeRegistration&) {},
                                 [](const score::mw::log::detail::SharedMemoryRecord&) {});
            return reader;
        });
        return factory;
    }

    score::os::UnistdMock unistd_mock;

    std::vector<score::mw::log::detail::Byte> buffer1{};
    std::vector<score::mw::log::detail::Byte> buffer2{};

    score::mw::log::detail::SharedData shared_data{};

    score::mw::log::detail::SharedMemoryWriter writer{shared_data, []() noexcept {}};
};

TEST_F(DataRouterTestFixture, ConstructionAndDestructionOnStack)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_server");
    RecordProperty("Description", "Check that a DataRouter can be constructed and destroyed on the stack.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger);
}

TEST_F(DataRouterTestFixture, ConstructionAndDestructionOnHeap)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_server");
    RecordProperty("Description", "Check that a DataRouter can be constructed and destroyed on the heap.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    auto unique_datarouter = std::make_unique<datarouter::DataRouter>(logger);
    unique_datarouter.reset();
}

TEST_F(DataRouterTestFixture, WhenOnlySourceSession)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that multiple source sessions can be created and enabled for DLT.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).Times(2).WillRepeatedly([](const score::mw::log::NvConfig& nv_cfg) {
        return std::make_unique<internal::LogParser>(nv_cfg);
    });

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session1_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto source_session1 = router.NewSourceSession(1001,
                                                   "source1",
                                                   kDefaultDltEnable,
                                                   std::move(session1_sender),
                                                   0.,
                                                   false,
                                                   2001,
                                                   nv_config,
                                                   CreateReaderFactory());
    EXPECT_EQ(source_session1->Tick(), false);
    router.ShowSourceStatistics(0);

    auto session2_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto source_session2 = router.NewSourceSession(1002,
                                                   "source2",
                                                   kDefaultDltEnable,
                                                   std::move(session2_sender),
                                                   0.,
                                                   false,
                                                   2001,
                                                   nv_config,
                                                   CreateReaderFactory());

    constexpr bool kEnableDltLogging{true};
    router.ForEachSource(kEnableDltLogging);
}

struct TestMessage
{
    int32_t test_field;
};

SCORE_STRUCT_VISITABLE(TestMessage, test_field)

class AnyHandlerMock : public LogParser::AnyHandler
{
  public:
    MOCK_METHOD(void, Handle, (const TypeInfo&, TimestampT, const char*, BufsizeT), (override final));
};

class TypeHandlerMock : public LogParser::TypeHandler
{
  public:
    MOCK_METHOD(void, Handle, (TimestampT, const char*, BufsizeT), (override final));
};

TEST_F(DataRouterTestFixture, WhenMessageHandledFromRw)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a message written via mw::log reaches the registered handlers.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    TestMessage message{2345};
    auto type_info = test::utils::CreateTypeInfo<TestMessage>();
    const auto register_result = writer.TryRegisterType(type_info);

    writer.AllocAndWrite(
        [&message](auto data_span) {
            return score::common::visitor::logging_serializer::serialize(
                message, data_span.data(), score::mw::log::detail::GetDataSizeAsLength(data_span));
        },
        register_result.value(),
        score::common::visitor::logging_serializer::serialize_size(message));

    shared_data.writer_detached.store(true);

    testing::StrictMock<AnyHandlerMock> any_handler;
    EXPECT_CALL(any_handler, Handle(_, _, _, _)).Times(1);

    testing::StrictMock<TypeHandlerMock> type_handler;
    EXPECT_CALL(type_handler, Handle(_, _, _)).Times(1);

    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).WillOnce([&](const score::mw::log::NvConfig& nv_cfg) {
        return std::make_unique<internal::LogParser>(
            nv_cfg,
            std::vector<ILogParser::AnyHandler*>{&any_handler},
            internal::LogParser::HandleRequestMap{{"score::platform::datarouter::TestMessage", &type_handler}});
    });

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto source_session1 = router.NewSourceSession(1001,
                                                   "source1",
                                                   kDefaultDltEnable,
                                                   std::move(session_sender),
                                                   0.,
                                                   false,
                                                   2001,
                                                   nv_config,
                                                   CreateReaderFactory());
    EXPECT_EQ(source_session1->Tick(), false);

    constexpr bool kEnableDltLogging{true};
    router.ForEachSource(kEnableDltLogging);
}

// To test a condition inside the Tick() method.
// The condition is:
// if (message_count_dropped_new != message_count_dropped_)
TEST_F(DataRouterTestFixture, TestDifferentCountsOfMessagesDropped)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a change in buffer-full drops is handled on tick.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger);

    auto session1_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto source_session1 = router.NewSourceSession(1001,
                                                   "source1",
                                                   kDefaultDltEnable,
                                                   std::move(session1_sender),
                                                   0.,
                                                   false,
                                                   2001,
                                                   nv_config,
                                                   CreateReaderFactory());
    EXPECT_EQ(source_session1->Tick(), false);

    // Set a different value for the dropped messages.
    shared_data.number_of_drops_buffer_full = 7;

    // Call tick again.
    EXPECT_EQ(source_session1->Tick(), false);
    router.ShowSourceStatistics(0);
}

// To test a condition inside the Tick() method.
// The condition is:
// if (message_count_dropped_invalid_size_new != message_count_dropped_invalid_size_)
TEST_F(DataRouterTestFixture, TestDifferentCountsOfInvalidSizeMessagesDropped)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a change in invalid-size drops is handled on tick.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger);

    auto session1_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto source_session1 = router.NewSourceSession(1001,
                                                   "source1",
                                                   kDefaultDltEnable,
                                                   std::move(session1_sender),
                                                   0.,
                                                   false,
                                                   2001,
                                                   nv_config,
                                                   CreateReaderFactory());
    EXPECT_EQ(source_session1->Tick(), false);

    // Set a different value for the dropped messages.
    shared_data.number_of_drops_invalid_size = 7;

    // Call tick again.
    EXPECT_EQ(source_session1->Tick(), false);
    router.ShowSourceStatistics(0);
}

TEST(DataRouterTest, QuotaValueAsStringMethodShallReturnunlimitedIfTheQuotaIsBiggerThanOrEqualToTheMax)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_bw_quota_config");
    RecordProperty("Description", "Check that a quota at the maximum value is shown as unlimited.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");

    const auto* expected_result{"[unlimited]"};
    double quota{};
    auto string_representative = datarouter::QuotaValueAsString(std::numeric_limits<decltype(quota)>::max());
    EXPECT_EQ(string_representative, expected_result);
}

TEST_F(DataRouterTestFixture, CreatingNewSessionShallFailIfTheFileDescriptorIsZero)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that no source session is created for file descriptor zero.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");

    testing::StrictMock<AnyHandlerMock> any_handler;
    testing::StrictMock<TypeHandlerMock> type_handler;
    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).Times(0);
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session =
        router.NewSourceSession(0, "src1", kDefaultDltEnable, std::move(session_sender), 0., false, 2001, nv_config);
    EXPECT_EQ(src_session, nullptr);
}

// This test is to cover the conditions of using the default stack and ring sizes in case of providing zeros,
// There is no expectation or assertion we can set to check the usage of the default stack and ring size.
TEST_F(DataRouterTestFixture, TestUsingTheDefaultStackAndRingSizesIfTheUserProvidesZeros)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_server");
    RecordProperty("Description", "Check that a DataRouter can be constructed with default sizes.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, std::move(factory_mock));
}

// To test the else case of the below condition and some inner conditions inside it for Tick() method.
// The condition is:
// else if ((acquire_requested_ == false) and (enabled_logging_at_server == true))
TEST_F(DataRouterTestFixture, TestSomeConditionsInsideTickMethodWithDisablingDlt)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_server");
    RecordProperty("Description", "Check that a source session can be created with DLT output disabled.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    testing::StrictMock<AnyHandlerMock> any_handler;
    testing::StrictMock<TypeHandlerMock> type_handler;
    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).WillOnce([&](const score::mw::log::NvConfig& nv_cfg) {
        return std::make_unique<internal::LogParser>(
            nv_cfg,
            std::vector<ILogParser::AnyHandler*>{&any_handler},
            internal::LogParser::HandleRequestMap{{"test::TestMessage", &type_handler}});
    });
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.NewSourceSession(0,
                                               "DR" /*Datarouter session*/,
                                               false /*disable dlt*/,
                                               std::move(session_sender),
                                               0.,
                                               false,
                                               2001,
                                               nv_config,
                                               CreateReaderFactory());
    EXPECT_NE(src_session, nullptr);
}

// To test the True case of the below condition and some inner conditions inside it for
// checkAndSetQuotaEnforcement() method.
// The condition is:
// if (!quotaOverlimitDetected_ && quotaEnforcementEnabled_)
TEST_F(DataRouterTestFixture, TestQuotaOverEnforcementWithZeroTimeDuration)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_quota_bw_reduction");
    RecordProperty("Description", "Check quota enforcement on a tick right after session creation.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");

    testing::StrictMock<AnyHandlerMock> any_handler;
    testing::StrictMock<TypeHandlerMock> type_handler;
    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).WillOnce([&](const score::mw::log::NvConfig& nv_cfg) {
        return std::make_unique<internal::LogParser>(
            nv_cfg,
            std::vector<ILogParser::AnyHandler*>{&any_handler},
            internal::LogParser::HandleRequestMap{{"test::TestMessage", &type_handler}});
    });
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.NewSourceSession(0,
                                               "DR",
                                               false,
                                               std::move(session_sender),
                                               0.,
                                               true /*Enable quota enforcement*/,
                                               2001,
                                               nv_config,
                                               CreateReaderFactory());

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->Tick(), false);
}

// To test the False case of the below condition and some inner conditions inside it for
// checkAndSetQuotaEnforcement() method.
// The condition is:
// if (tstat_in_msec == 0)
TEST_F(DataRouterTestFixture, TestQuotaEnforcementWithTimeDurationBiggerThanZero)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_quota_bw_reduction");
    RecordProperty("Description", "Check quota enforcement on a tick after time has passed.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    testing::StrictMock<AnyHandlerMock> any_handler;
    testing::StrictMock<TypeHandlerMock> type_handler;
    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).WillOnce([&](const score::mw::log::NvConfig& nv_cfg) {
        return std::make_unique<internal::LogParser>(
            nv_cfg,
            std::vector<ILogParser::AnyHandler*>{&any_handler},
            internal::LogParser::HandleRequestMap{{"test::TestMessage", &type_handler}});
    });
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.NewSourceSession(0,
                                               "DR",
                                               false,
                                               std::move(session_sender),
                                               0.,
                                               true /*Enable quota enforcement*/,
                                               2001,
                                               nv_config,
                                               CreateReaderFactory());
    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session' method and the invoking of the 'checkAndSetQuotaEnforcement' method to test
    // the quota enforcement.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->Tick(), false);
}

TEST_F(DataRouterTestFixture, OnSessionDetached)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a session closed by its peer is detached on tick.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    testing::StrictMock<AnyHandlerMock> any_handler;
    testing::StrictMock<TypeHandlerMock> type_handler;
    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).WillOnce([&](const score::mw::log::NvConfig& nv_cfg) {
        return std::make_unique<internal::LogParser>(
            nv_cfg,
            std::vector<ILogParser::AnyHandler*>{&any_handler},
            internal::LogParser::HandleRequestMap{{"test::TestMessage", &type_handler}});
    });
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.NewSourceSession(
        0, "DR", false, std::move(session_sender), 0., true, 2001, nv_config, CreateReaderFactory());
    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session'
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_NE(src_session, nullptr);
    src_session->OnClosedByPeer();  // simulate detached session
    EXPECT_EQ(src_session->Tick(), false);

    // another tick for already detached session
    EXPECT_EQ(src_session->Tick(), false);
}

TEST_F(DataRouterTestFixture, OnSessionDetachedWithError)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_verbose_messages");
    RecordProperty("Description", "Check that a session closed by its peer is detached when its reader fails.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    testing::StrictMock<AnyHandlerMock> any_handler;
    testing::StrictMock<TypeHandlerMock> type_handler;
    auto factory_mock = std::make_unique<testing::NiceMock<internal::ILogParserFactoryMock>>();
    EXPECT_CALL(*factory_mock, Create(_)).WillOnce([&](const score::mw::log::NvConfig& nv_cfg) {
        return std::make_unique<internal::LogParser>(
            nv_cfg,
            std::vector<ILogParser::AnyHandler*>{&any_handler},
            internal::LogParser::HandleRequestMap{{"test::TestMessage", &type_handler}});
    });
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, std::move(factory_mock));

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.NewSourceSession(
        0, "DR", false, std::move(session_sender), 0., true, 2001, nv_config, CreateReaderFactoryWithDetachedReader());
    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session'
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_NE(src_session, nullptr);
    src_session->OnClosedByPeer();  // simulate detached session
    EXPECT_EQ(src_session->Tick(), false);
}

// To test the True case of the below condition for checkAndSetQuotaEnforcement() method.
// The condition is:
// if (rate_KBps > quota_KBps_)
TEST_F(DataRouterTestFixture, TestQuotaOverLimitationWithTimeDurationBiggerThanZero)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_quota_bw_reduction");
    RecordProperty("Description", "Check that exceeding a zero quota is detected.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "boundary-values");

    TestMessage message{2345};
    auto type_info = test::utils::CreateTypeInfo<TestMessage>();
    const auto register_result = writer.TryRegisterType(type_info);

    const auto time_now = score::mw::log::detail::TimePoint::clock::now();
    writer.AllocAndWrite(time_now,
                         register_result.value(),
                         score::common::visitor::logging_serializer::serialize_size(message),
                         [&message](auto data_span) {
                             return score::common::visitor::logging_serializer::serialize(
                                 message, data_span.data(), score::mw::log::detail::GetDataSizeAsLength(data_span));
                         });

    shared_data.writer_detached.store(true);

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.NewSourceSession(0,
                                               "any app",
                                               false,
                                               std::move(session_sender),
                                               0. /*zero quota KB*/,
                                               true /*Enable quota enforcement*/,
                                               2001,
                                               nv_config,
                                               CreateReaderFactory());

    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session' method and the invoking of the 'checkAndSetQuotaEnforcement' method to test
    // the quota over limitation.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->Tick(), false);
    // Call 'tick' again to invoke 'checkAndSetQuotaEnforcement' method after updating total size with the new value.
    EXPECT_EQ(src_session->Tick(), false);
}

// To test the True case of the below condition for show_stats() method.
// The condition is:
// if (rate_KBps > quota_KBps_ && quotaEnforcementEnabled_)
TEST_F(DataRouterTestFixture, TestQuotaLimitationWithTimeDurationBiggerThanZeroWithQuotaEnforcementEnabled)
{
    RecordProperty("PartiallyVerifies", "comp_req__data_router__dlt_quota_bw_reduction");
    RecordProperty("Description", "Check that statistics are shown for a session over its enforced quota.");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "requirements-analysis");

    TestMessage message{2345};
    auto type_info = test::utils::CreateTypeInfo<TestMessage>();
    const auto register_result = writer.TryRegisterType(type_info);

    const auto time_now = score::mw::log::detail::TimePoint::clock::now();
    writer.AllocAndWrite(time_now,
                         register_result.value(),
                         score::common::visitor::logging_serializer::serialize_size(message),
                         [&message](auto data_span) {
                             return score::common::visitor::logging_serializer::serialize(
                                 message, data_span.data(), score::mw::log::detail::GetDataSizeAsLength(data_span));
                         });

    shared_data.writer_detached.store(true);

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.NewSourceSession(0,
                                               "any app",
                                               false,
                                               std::move(session_sender),
                                               0. /*zero quota KB*/,
                                               true /*Enable quota enforcement*/,
                                               2001,
                                               nv_config,
                                               CreateReaderFactory());

    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session' method and the invoking of the 'checkAndSetQuotaEnforcement' method to test
    // the quota over limitation.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->Tick(), false);
    // Call 'tick' again to invoke 'checkAndSetQuotaEnforcement' method after updating total size with the new value.
    EXPECT_EQ(src_session->Tick(), false);
    // Calling 'show_source_statistics' method to invoke 'show_stats' method.
    router.ShowSourceStatistics(0);
}

}  // namespace datarouter
}  // namespace platform
}  // namespace score
