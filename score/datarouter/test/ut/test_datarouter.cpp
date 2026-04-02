#include "score/datarouter/datarouter/data_router.h"

#include "logparser/logparser.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/mw/log/detail/data_router/shared_memory/reader_factory_mock.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_writer.h"
#include "score/datarouter/daemon_communication/session_handle_mock.h"
#include "score/datarouter/test/utils/data_router_test_utils.h"

#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/configuration/nvconfigfactory.h"

#include "score/static_reflection_with_serialization/serialization/include/serialization/visit_size.h"

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader_mock.h"

#include <sstream>

#include "gtest/gtest.h"

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
    using DataRouter::SourceSession::execute_send_no_operation;
    using DataRouter::SourceSession::is_source_closed;
    using DataRouter::SourceSession::on_acquire_response;
    using DataRouter::SourceSession::process_detached_logs;
    using DataRouter::SourceSession::processAndRouteLogMessages;
    using DataRouter::SourceSession::request_acquire;
    using DataRouter::SourceSession::schedule_send_check_alive;
    using DataRouter::SourceSession::SourceSession;
    using DataRouter::SourceSession::tick;
    using DataRouter::SourceSession::tryFinalizeAcquisition;

    using DataRouter::SourceSession::command_data_;
    using DataRouter::SourceSession::command_send_check_alive_;
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
                         std::unique_ptr<score::platform::internal::LogParser> parser = nullptr)
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
using score::platform::datarouter::CommandData;
using score::platform::datarouter::LocalSubscriberData;
using score::platform::datarouter::StatsData;
using score::platform::datarouter::synchronized;

class SourceSessionTestFixture : public ::testing::Test
{
  public:
    SourceSessionTestFixture() : logger_(score::mw::log::CreateLogger("DRUT", "statistics")), router_(logger_) {}

  public:
    void SetUp() override
    {
        auto sharedMemoryReaderMock = std::make_unique<score::mw::log::detail::ISharedMemoryReaderMock>();
        sharedMemoryReaderMock_ = sharedMemoryReaderMock.get();
        score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> sessionHandleMock =
            score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());

        auto nv_config = score::mw::log::NvConfigFactory::CreateEmpty();
        auto parser = std::make_unique<LogParser>(nv_config);

        sourceSessionUnderTest_ = std::make_unique<DataRouter::DataRouterForTest::SourceSession::SourceSessionForTest>(
            router_,
            std::move(sharedMemoryReaderMock),
            "source1",
            false,
            std::move(sessionHandleMock),
            0,
            false,
            logger_,
            std::move(parser));
    }

  protected:
    daemon::mock::SessionHandleMock* SetupMockSessionHandle()
    {
        auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
        auto* session_sender_ptr = session_sender.get();
        score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle_base(std::move(session_sender));
        sourceSessionUnderTest_->handle_ = std::move(handle_base);
        return session_sender_ptr;
    }

    score::mw::log::Logger& logger_;
    datarouter::DataRouter router_;
    score::mw::log::detail::ISharedMemoryReaderMock* sharedMemoryReaderMock_;

    std::unique_ptr<DataRouter::DataRouterForTest::SourceSession::SourceSessionForTest> sourceSessionUnderTest_;
};

TEST_F(SourceSessionTestFixture, TryFinalizeAcquisitionIsBlockReleased)
{
    bool needs_fast_reschedule = true;

    EXPECT_CALL(*sharedMemoryReaderMock_, IsBlockReleasedByWriters(_)).WillOnce(Return(true));

    // any value except null
    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        cmd.data_acquired_ = score::mw::log::detail::ReadAcquireResult();
    });

    EXPECT_TRUE(sourceSessionUnderTest_->tryFinalizeAcquisition(needs_fast_reschedule));
}

TEST_F(SourceSessionTestFixture, TryFinalizeAcquisitionIsBlockNotReleased)
{
    bool needs_fast_reschedule = true;

    EXPECT_CALL(*sharedMemoryReaderMock_, IsBlockReleasedByWriters(_)).WillOnce(Return(false));

    // any value except null
    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        cmd.data_acquired_ = score::mw::log::detail::ReadAcquireResult();
    });

    EXPECT_FALSE(sourceSessionUnderTest_->tryFinalizeAcquisition(needs_fast_reschedule));
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesAcquireFinalized)
{
    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = true;
    bool needs_fast_reschedule = false;

    sourceSessionUnderTest_->processAndRouteLogMessages(message_count_local,
                                                        transport_delay_local,
                                                        number_of_bytes_in_buffer,
                                                        acquire_finalized_in_this_tick,
                                                        needs_fast_reschedule);

    // ASSERT_TRUE(sourceSessionUnderTest_->command_data_);

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        EXPECT_FALSE(cmd.acquire_requested);
        EXPECT_EQ(cmd.ticks_without_write, 0);
    });
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesBlockExpectedToBeNextDataAvailable)
{
    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        cmd.acquire_requested = false;
        cmd.command_detach_on_closed = false;
        cmd.block_expected_to_be_next = 0;
    });

    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& cmd) {
        cmd.enabled_logging_at_server_ = true;
    });

    EXPECT_CALL(*sharedMemoryReaderMock_, PeekNumberOfBytesAcquiredInBuffer(_)).WillOnce(Return(10));

    sourceSessionUnderTest_->processAndRouteLogMessages(message_count_local,
                                                        transport_delay_local,
                                                        number_of_bytes_in_buffer,
                                                        acquire_finalized_in_this_tick,
                                                        needs_fast_reschedule);

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        EXPECT_TRUE(cmd.acquire_requested);
    });
    EXPECT_TRUE(needs_fast_reschedule);
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesBlockExpectedToBeNextDataNotAvailable)
{
    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    const uint8_t ticks_without_write = 1;

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        cmd.acquire_requested = false;
        cmd.command_detach_on_closed = false;
        cmd.block_expected_to_be_next = 0;
        cmd.ticks_without_write = ticks_without_write;
    });
    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& cmd) {
        cmd.enabled_logging_at_server_ = true;
    });

    EXPECT_CALL(*sharedMemoryReaderMock_, PeekNumberOfBytesAcquiredInBuffer(_)).WillOnce(Return(std::nullopt));

    sourceSessionUnderTest_->processAndRouteLogMessages(message_count_local,
                                                        transport_delay_local,
                                                        number_of_bytes_in_buffer,
                                                        acquire_finalized_in_this_tick,
                                                        needs_fast_reschedule);

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        EXPECT_EQ(cmd.ticks_without_write, ticks_without_write + 1);
    });
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesNotBlockExpectedToBeNext)
{
    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        cmd.acquire_requested = false;
        cmd.command_detach_on_closed = false;
        cmd.block_expected_to_be_next = std::nullopt;
    });
    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& cmd) {
        cmd.enabled_logging_at_server_ = true;
    });

    sourceSessionUnderTest_->processAndRouteLogMessages(message_count_local,
                                                        transport_delay_local,
                                                        number_of_bytes_in_buffer,
                                                        acquire_finalized_in_this_tick,
                                                        needs_fast_reschedule);

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        EXPECT_TRUE(cmd.acquire_requested);
    });
    EXPECT_TRUE(needs_fast_reschedule);
}

TEST_F(SourceSessionTestFixture, RequestAcquire)
{
    UnixDomainServer::SessionHandle session_handle(0, nullptr);

    sourceSessionUnderTest_->handle_ = session_handle;

    // verify that DataRouter::SourceSession is able
    // to process UnixDomainServer::SessionHandle type
    EXPECT_NO_THROW(sourceSessionUnderTest_->request_acquire());
}

TEST_F(SourceSessionTestFixture, ScheduleSendCheckAlive)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Verification of the control flow and data flow");
    RecordProperty("Description", "Verify schedule_send_check_alive sets the check alive flag to true.");
    RecordProperty("DerivationTechnique", "Equivalence partitioning");

    // Initially the flag should be false
    EXPECT_FALSE(sourceSessionUnderTest_->command_send_check_alive_.load());

    // Call the method
    sourceSessionUnderTest_->schedule_send_check_alive();

    // Verify the flag is now true
    EXPECT_TRUE(sourceSessionUnderTest_->command_send_check_alive_.load());
}

TEST_F(SourceSessionTestFixture, OnAcquireResponse)
{
    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        cmd.data_acquired_ = std::nullopt;
    });

    const score::mw::log::detail::ReadAcquireResult readAcquireResult{44};
    sourceSessionUnderTest_->on_acquire_response(readAcquireResult);

    sourceSessionUnderTest_->command_data_.with_lock([&readAcquireResult](auto& cmd) {
        ASSERT_TRUE(cmd.data_acquired_);
        EXPECT_EQ(cmd.data_acquired_->acquired_buffer, readAcquireResult.acquired_buffer);
    });
}

TEST_F(SourceSessionTestFixture, ExecuteSendNoOperationWithUnixDomainSocket)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Verification of the control flow and data flow");
    RecordProperty("Description", "Verify execute_send_no_operation is a no-op for Unix domain sockets.");
    RecordProperty("DerivationTechnique", "Equivalence partitioning");

    UnixDomainServer::SessionHandle session_handle(0, nullptr);
    sourceSessionUnderTest_->handle_ = session_handle;

    // Should not throw and is a no-op for Unix domain sockets
    EXPECT_NO_THROW(sourceSessionUnderTest_->execute_send_no_operation());
}

TEST_F(SourceSessionTestFixture, ExecuteSendNoOperationWithSessionHandleMockLoggingDisabled)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Verification of the control flow and data flow");
    RecordProperty("Description",
                   "Verify execute_send_no_operation with mock session handle when logging is disabled.");
    RecordProperty("DerivationTechnique", "Equivalence partitioning");

    auto* session_sender_ptr = SetupMockSessionHandle();

    // Set logging disabled
    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& data) {
        data.enabled_logging_at_server_ = false;
    });

    // Expect SendCheckAlive to be called when logging is disabled
    EXPECT_CALL(*session_sender_ptr, SendCheckAlive()).Times(1);

    sourceSessionUnderTest_->execute_send_no_operation();
}

TEST_F(SourceSessionTestFixture, ExecuteSendNoOperationWithSessionHandleMockLoggingEnabled)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Verification of the control flow and data flow");
    RecordProperty("Description", "Verify execute_send_no_operation sends command when logging is enabled.");
    RecordProperty("DerivationTechnique", "Equivalence partitioning");

    auto* session_sender_ptr = SetupMockSessionHandle();

    // Set logging enabled
    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& data) {
        data.enabled_logging_at_server_ = true;
    });

    // Expect SendCheckAlive NOT to be called when logging is enabled
    EXPECT_CALL(*session_sender_ptr, SendCheckAlive()).Times(0);

    sourceSessionUnderTest_->execute_send_no_operation();
}

TEST_F(SourceSessionTestFixture, TickCallsExecuteSendNoOperationWhenFlagIsSet)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Verification of the control flow and data flow");
    RecordProperty("Description", "Verify tick calls execute_send_no_operation when check alive flag is set.");
    RecordProperty("DerivationTechnique", "Equivalence partitioning");

    auto* session_sender_ptr = SetupMockSessionHandle();

    // Set logging disabled so SendCheckAlive will be called
    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& data) {
        data.enabled_logging_at_server_ = false;
    });

    // Schedule the check alive which sets the flag to true
    sourceSessionUnderTest_->schedule_send_check_alive();
    EXPECT_TRUE(sourceSessionUnderTest_->command_send_check_alive_.load());

    // Expect SendCheckAlive to be called during tick
    EXPECT_CALL(*session_sender_ptr, SendCheckAlive()).Times(1);

    // Call tick which should execute_send_no_operation and reset the flag
    sourceSessionUnderTest_->tick();

    // Verify the flag was reset to false after tick
    EXPECT_FALSE(sourceSessionUnderTest_->command_send_check_alive_.load());
}

TEST_F(SourceSessionTestFixture, TickDoesNotCallExecuteSendNoOperationWhenFlagIsNotSet)
{
    RecordProperty("ASIL", "QM");
    RecordProperty("TestType", "Verification of the control flow and data flow");
    RecordProperty("Description",
                   "Verify tick does not call execute_send_no_operation when check alive flag is not set.");
    RecordProperty("DerivationTechnique", "Equivalence partitioning");

    auto* session_sender_ptr = SetupMockSessionHandle();

    // Set logging disabled
    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& data) {
        data.enabled_logging_at_server_ = false;
    });

    // Verify flag is initially false
    EXPECT_FALSE(sourceSessionUnderTest_->command_send_check_alive_.load());

    // Expect SendCheckAlive NOT to be called
    EXPECT_CALL(*session_sender_ptr, SendCheckAlive()).Times(0);

    // Call tick - should not call execute_send_no_operation
    sourceSessionUnderTest_->tick();

    // Flag should still be false
    EXPECT_FALSE(sourceSessionUnderTest_->command_send_check_alive_.load());
}

TEST_F(SourceSessionTestFixture, IsSourceClosed)
{
    {
        sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& data) {
            data.detach_on_closed_processed = false;
        });

        EXPECT_FALSE(sourceSessionUnderTest_->is_source_closed());
    }
    {
        sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& data) {
            data.detach_on_closed_processed = true;
        });

        EXPECT_TRUE(sourceSessionUnderTest_->is_source_closed());
    }
}

// custom action because InvokeArgument could not work with reference to move-only argument
ACTION(InvokeSharedReadArguments)
{
    arg0(score::mw::log::detail::TypeRegistration());
    arg1(score::mw::log::detail::SharedMemoryRecord());
}

TEST_F(SourceSessionTestFixture, ProcessDetachedLogsReaderUsed)
{
    score::mw::log::detail::TypeRegistration tr;

    EXPECT_CALL(*sharedMemoryReaderMock_, ReadDetached(_, _))
        .WillOnce(DoAll(InvokeSharedReadArguments(), Return(std::nullopt)));

    uint64_t number_of_bytes_in_buffer = 0;
    sourceSessionUnderTest_->process_detached_logs(number_of_bytes_in_buffer);
}

TEST_F(SourceSessionTestFixture, ProcessAndRouteLogMessagesQuotaLimitExceeded)
{
    uint64_t message_count_local = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    uint64_t number_of_bytes_in_buffer = 0;
    bool acquire_finalized_in_this_tick = false;
    bool needs_fast_reschedule = false;

    sourceSessionUnderTest_->command_data_.with_lock([](auto& cmd) {
        cmd.acquire_requested = false;
        cmd.command_detach_on_closed = false;
        cmd.block_expected_to_be_next = std::nullopt;
    });
    sourceSessionUnderTest_->local_subscriber_data_.with_lock([](auto& cmd) {
        cmd.enabled_logging_at_server_ = true;
    });

    sourceSessionUnderTest_->stats_data_.with_lock([](auto& cmd) {
        cmd.quota_overlimit_detected = true;
    });

    EXPECT_CALL(*sharedMemoryReaderMock_, Read(_, _))
        .WillOnce(DoAll(InvokeSharedReadArguments(), Return(std::nullopt)));

    sourceSessionUnderTest_->processAndRouteLogMessages(message_count_local,
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
        score::os::Unistd::set_testing_instance(unistd_mock_);

        constexpr auto kBufferSize = UINT64_C(1024);
        buffer1_.resize(kBufferSize);
        buffer2_.resize(kBufferSize);
        shared_data_.control_block.control_block_even.data = {buffer1_.data(), static_cast<int64_t>(kBufferSize)};
        shared_data_.control_block.control_block_odd.data = {buffer2_.data(), static_cast<int64_t>(kBufferSize)};
    }

    void TearDown() override
    {
        score::os::Unistd::restore_instance();
    }

    score::mw::log::NvConfig nvConfig = score::mw::log::NvConfigFactory::CreateEmpty();

    datarouter::DataRouter::LogParserFactory DefaultParserFactory()
    {
        return [this]() -> std::unique_ptr<LogParser> {
            return std::make_unique<LogParser>(nvConfig);
        };
    }

    score::mw::log::detail::ReaderFactoryPtr CreateReaderFactory()
    {
        auto factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
        EXPECT_CALL(*factory, Create(_, _)).WillOnce([this](auto, auto) -> auto {
            return std::make_unique<score::mw::log::detail::SharedMemoryReader>(
                shared_data_,
                score::mw::log::detail::AlternatingReadOnlyReader{shared_data_.control_block, buffer1_, buffer2_},
                []() noexcept {});
        });
        return factory;
    }

    score::mw::log::detail::ReaderFactoryPtr CreateReaderFactoryWithDetachedReader()
    {
        auto factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
        EXPECT_CALL(*factory, Create(_, _)).WillOnce([this](auto, auto) -> auto {
            auto reader = std::make_unique<score::mw::log::detail::SharedMemoryReader>(
                shared_data_,
                score::mw::log::detail::AlternatingReadOnlyReader{shared_data_.control_block, buffer1_, buffer2_},
                []() noexcept {});
            // detach reader to simulate read error on next read
            reader->ReadDetached([](const score::mw::log::detail::TypeRegistration&) {},
                                 [](const score::mw::log::detail::SharedMemoryRecord&) {});
            return reader;
        });
        return factory;
    }

    score::os::UnistdMock unistd_mock_;

    std::vector<score::mw::log::detail::Byte> buffer1_{};
    std::vector<score::mw::log::detail::Byte> buffer2_{};

    score::mw::log::detail::SharedData shared_data_{};

    score::mw::log::detail::SharedMemoryWriter writer_{shared_data_, []() noexcept {}};
};

TEST_F(DataRouterTestFixture, ConstructionAndDestructionOnStack)
{
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger);
}

TEST_F(DataRouterTestFixture, ConstructionAndDestructionOnHeap)
{
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    auto unique_datarouter = std::make_unique<datarouter::DataRouter>(logger);
    unique_datarouter.reset();
}

TEST_F(DataRouterTestFixture, WhenOnlySourceSession)
{
    int sourceCallbackCount = 0;
    auto parserFactory = [&sourceCallbackCount]() -> std::unique_ptr<LogParser> {
        ++sourceCallbackCount;
        auto nv_config = score::mw::log::NvConfigFactory::CreateEmpty();
        return std::make_unique<LogParser>(nv_config);
    };

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger, parserFactory);

    auto session1_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto sourceSession1 = router.new_source_session(
        1001, "source1", kDefaultDltEnable, std::move(session1_sender), 0., false, 2001, CreateReaderFactory());
    EXPECT_EQ(sourceSession1->tick(), false);
    router.show_source_statistics(0);

    auto session2_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto sourceSession2 = router.new_source_session(
        1002, "source2", kDefaultDltEnable, std::move(session2_sender), 0., false, 2001, CreateReaderFactory());

    int sourceListItemCount = 0;
    int sourceListEndCount = 0;
    auto e = [&](LogParser&) {
        ++sourceListItemCount;
    };
    auto f = [&] {
        ++sourceListEndCount;
    };
    constexpr bool enable_dlt_logging{true};
    router.for_each_source_parser(e, f, enable_dlt_logging);
    EXPECT_EQ(sourceListItemCount, 2);
    EXPECT_EQ(sourceListEndCount, 1);
    EXPECT_EQ(sourceCallbackCount, 2);  // factory called once per session
}

struct TestMessage
{
    int32_t testField;
};

STRUCT_VISITABLE(TestMessage, testField)

class AnyHandlerMock : public LogParser::AnyHandler
{
  public:
    MOCK_METHOD(void, handle, (const TypeInfo&, timestamp_t, const char*, bufsize_t), (override final));
};

class TypeHandlerMock : public LogParser::TypeHandler
{
  public:
    MOCK_METHOD(void, handle, (timestamp_t, const char*, bufsize_t), (override final));
};

TEST_F(DataRouterTestFixture, WhenMessageHandledFromRw)
{
    TestMessage message{2345};
    auto type_info = utils::CreateTypeInfo<TestMessage>();
    const auto register_result = writer_.TryRegisterType(type_info);

    writer_.AllocAndWrite(
        [&message](auto data_span) {
            return score::common::visitor::logging_serializer::serialize(
                message, data_span.data(), score::mw::log::detail::GetDataSizeAsLength(data_span));
        },
        register_result.value(),
        score::common::visitor::logging_serializer::serialize_size(message));

    shared_data_.writer_detached.store(true);

    testing::StrictMock<AnyHandlerMock> anyHandler;
    EXPECT_CALL(anyHandler, handle(_, _, _, _)).Times(1);

    testing::StrictMock<TypeHandlerMock> typeHandler;
    EXPECT_CALL(typeHandler, handle(_, _, _)).Times(1);

    auto nv_config_local = score::mw::log::NvConfigFactory::CreateEmpty();
    auto parserFactory = [&anyHandler, &typeHandler, &nv_config_local]() -> std::unique_ptr<LogParser> {
        return std::make_unique<LogParser>(
            nv_config_local,
            std::vector<LogParser::AnyHandler*>{&anyHandler},
            std::vector<LogParser::TypeHandlerBinding>{{"score::platform::datarouter::TestMessage", &typeHandler}});
    };

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger, parserFactory);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto sourceSession1 = router.new_source_session(
        1001, "source1", kDefaultDltEnable, std::move(session_sender), 0., false, 2001, CreateReaderFactory());
    EXPECT_EQ(sourceSession1->tick(), false);

    int sourceListItemCount = 0;
    int sourceListEndCount = 0;
    auto e = [&](LogParser& parser) {
        std::ignore = parser;
        ++sourceListItemCount;
    };
    auto f = [&] {
        ++sourceListEndCount;
    };
    constexpr bool enable_dlt_logging{true};
    router.for_each_source_parser(e, f, enable_dlt_logging);
    EXPECT_EQ(sourceListItemCount, 1);
    EXPECT_EQ(sourceListEndCount, 1);
}

// To test a condition inside the tick() method.
// The condition is:
// if (message_count_dropped_new != message_count_dropped_)
TEST_F(DataRouterTestFixture, TestDifferentCountsOfMessagesDropped)
{
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger, DefaultParserFactory());

    auto session1_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto sourceSession1 = router.new_source_session(
        1001, "source1", kDefaultDltEnable, std::move(session1_sender), 0., false, 2001, CreateReaderFactory());
    EXPECT_EQ(sourceSession1->tick(), false);

    // Set a different value for the dropped messages.
    shared_data_.number_of_drops_buffer_full = 7;

    // Call tick again.
    EXPECT_EQ(sourceSession1->tick(), false);
    router.show_source_statistics(0);
}

// To test a condition inside the tick() method.
// The condition is:
// if (message_count_dropped_invalid_size_new != message_count_dropped_invalid_size_)
TEST_F(DataRouterTestFixture, TestDifferentCountsOfInvalidSizeMessagesDropped)
{
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");
    datarouter::DataRouter router(logger, DefaultParserFactory());

    auto session1_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto sourceSession1 = router.new_source_session(
        1001, "source1", kDefaultDltEnable, std::move(session1_sender), 0., false, 2001, CreateReaderFactory());
    EXPECT_EQ(sourceSession1->tick(), false);

    // Set a different value for the dropped messages.
    shared_data_.number_of_drops_invalid_size = 7;

    // Call tick again.
    EXPECT_EQ(sourceSession1->tick(), false);
    router.show_source_statistics(0);
}

TEST(DataRouterTest, QuotaValueAsStringMethodShallReturnunlimitedIfTheQuotaIsBiggerThanOrEqualToTheMax)
{
    auto expected_result{"[unlimited]"};
    double quota{};
    auto string_representative = datarouter::QuotaValueAsString(std::numeric_limits<decltype(quota)>::max());
    EXPECT_EQ(string_representative, expected_result);
}

TEST_F(DataRouterTestFixture, CreatingNewSessionShallFailIfTheFileDescriptorIsZero)
{
    testing::StrictMock<AnyHandlerMock> anyHandler;
    testing::StrictMock<TypeHandlerMock> typeHandler;
    auto nv_config_local = score::mw::log::NvConfigFactory::CreateEmpty();
    auto parserFactory = [&anyHandler, &typeHandler, &nv_config_local]() -> std::unique_ptr<LogParser> {
        return std::make_unique<LogParser>(
            nv_config_local,
            std::vector<LogParser::AnyHandler*>{&anyHandler},
            std::vector<LogParser::TypeHandlerBinding>{{"test::TestMessage", &typeHandler}});
    };
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, parserFactory);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session =
        router.new_source_session(0, "src1", kDefaultDltEnable, std::move(session_sender), 0., false, 2001);
    EXPECT_EQ(src_session, nullptr);
}

// This test is to cover the conditions of using the default stack and ring sizes in case of providing zeros,
// There is no expectation or assertion we can set to check the usage of the default stack and ring size.
TEST_F(DataRouterTestFixture, TestUsingTheDefaultStackAndRingSizesIfTheUserProvidesZeros)
{
    int sourceCallbackCount = 0;
    auto parserFactory = [&sourceCallbackCount]() -> std::unique_ptr<LogParser> {
        ++sourceCallbackCount;
        auto nv_config = score::mw::log::NvConfigFactory::CreateEmpty();
        return std::make_unique<LogParser>(nv_config);
    };

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, parserFactory);
}

// To test the else case of the below condition and some inner conditions inside it for tick() method.
// The condition is:
// else if ((acquire_requested_ == false) and (enabled_logging_at_server_ == true))
TEST_F(DataRouterTestFixture, TestSomeConditionsInsideTickMethodWithDisablingDlt)
{
    testing::StrictMock<AnyHandlerMock> anyHandler;
    testing::StrictMock<TypeHandlerMock> typeHandler;
    auto nv_config_local = score::mw::log::NvConfigFactory::CreateEmpty();
    auto parserFactory = [&anyHandler, &typeHandler, &nv_config_local]() -> std::unique_ptr<LogParser> {
        return std::make_unique<LogParser>(
            nv_config_local,
            std::vector<LogParser::AnyHandler*>{&anyHandler},
            std::vector<LogParser::TypeHandlerBinding>{{"test::TestMessage", &typeHandler}});
    };
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, parserFactory);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.new_source_session(0,
                                                 "DR" /*Datarouter session*/,
                                                 false /*disable dlt*/,
                                                 std::move(session_sender),
                                                 0.,
                                                 false,
                                                 2001,
                                                 CreateReaderFactory());
    EXPECT_NE(src_session, nullptr);
}

// To test the True case of the below condition and some inner conditions inside it for
// checkAndSetQuotaEnforcement() method.
// The condition is:
// if (!quotaOverlimitDetected_ && quotaEnforcementEnabled_)
TEST_F(DataRouterTestFixture, TestQuotaOverEnforcementWithZeroTimeDuration)
{
    testing::StrictMock<AnyHandlerMock> anyHandler;
    testing::StrictMock<TypeHandlerMock> typeHandler;
    auto nv_config_local = score::mw::log::NvConfigFactory::CreateEmpty();
    auto parserFactory = [&anyHandler, &typeHandler, &nv_config_local]() -> std::unique_ptr<LogParser> {
        return std::make_unique<LogParser>(
            nv_config_local,
            std::vector<LogParser::AnyHandler*>{&anyHandler},
            std::vector<LogParser::TypeHandlerBinding>{{"test::TestMessage", &typeHandler}});
    };
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, parserFactory);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.new_source_session(
        0, "DR", false, std::move(session_sender), 0., true /*Enable quota enforcement*/, 2001, CreateReaderFactory());

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->tick(), false);
}

// To test the False case of the below condition and some inner conditions inside it for
// checkAndSetQuotaEnforcement() method.
// The condition is:
// if (tstat_in_msec == 0)
TEST_F(DataRouterTestFixture, TestQuotaEnforcementWithTimeDurationBiggerThanZero)
{
    testing::StrictMock<AnyHandlerMock> anyHandler;
    testing::StrictMock<TypeHandlerMock> typeHandler;
    auto nv_config_local = score::mw::log::NvConfigFactory::CreateEmpty();
    auto parserFactory = [&anyHandler, &typeHandler, &nv_config_local]() -> std::unique_ptr<LogParser> {
        return std::make_unique<LogParser>(
            nv_config_local,
            std::vector<LogParser::AnyHandler*>{&anyHandler},
            std::vector<LogParser::TypeHandlerBinding>{{"test::TestMessage", &typeHandler}});
    };
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, parserFactory);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.new_source_session(
        0, "DR", false, std::move(session_sender), 0., true /*Enable quota enforcement*/, 2001, CreateReaderFactory());
    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session' method and the invoking of the 'checkAndSetQuotaEnforcement' method to test
    // the quota enforcement.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->tick(), false);
}

TEST_F(DataRouterTestFixture, OnSessionDetached)
{
    testing::StrictMock<AnyHandlerMock> anyHandler;
    testing::StrictMock<TypeHandlerMock> typeHandler;
    auto nv_config_local = score::mw::log::NvConfigFactory::CreateEmpty();
    auto parserFactory = [&anyHandler, &typeHandler, &nv_config_local]() -> std::unique_ptr<LogParser> {
        return std::make_unique<LogParser>(
            nv_config_local,
            std::vector<LogParser::AnyHandler*>{&anyHandler},
            std::vector<LogParser::TypeHandlerBinding>{{"test::TestMessage", &typeHandler}});
    };
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, parserFactory);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session =
        router.new_source_session(0, "DR", false, std::move(session_sender), 0., true, 2001, CreateReaderFactory());
    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session'
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_NE(src_session, nullptr);
    src_session->on_closed_by_peer();  // simulate detached session
    EXPECT_EQ(src_session->tick(), false);

    // another tick for already detached session
    EXPECT_EQ(src_session->tick(), false);
}

TEST_F(DataRouterTestFixture, OnSessionDetachedWithError)
{
    testing::StrictMock<AnyHandlerMock> anyHandler;
    testing::StrictMock<TypeHandlerMock> typeHandler;
    auto nv_config_local = score::mw::log::NvConfigFactory::CreateEmpty();
    auto parserFactory = [&anyHandler, &typeHandler, &nv_config_local]() -> std::unique_ptr<LogParser> {
        return std::make_unique<LogParser>(
            nv_config_local,
            std::vector<LogParser::AnyHandler*>{&anyHandler},
            std::vector<LogParser::TypeHandlerBinding>{{"test::TestMessage", &typeHandler}});
    };
    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, parserFactory);

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.new_source_session(
        0, "DR", false, std::move(session_sender), 0., true, 2001, CreateReaderFactoryWithDetachedReader());
    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session'
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ASSERT_NE(src_session, nullptr);
    src_session->on_closed_by_peer();  // simulate detached session
    EXPECT_EQ(src_session->tick(), false);
}

// To test the True case of the below condition for checkAndSetQuotaEnforcement() method.
// The condition is:
// if (rate_KBps > quota_KBps_)
TEST_F(DataRouterTestFixture, TestQuotaOverLimitationWithTimeDurationBiggerThanZero)
{
    TestMessage message{2345};
    auto type_info = utils::CreateTypeInfo<TestMessage>();
    const auto register_result = writer_.TryRegisterType(type_info);

    const auto time_now = score::mw::log::detail::TimePoint::clock::now();
    writer_.AllocAndWrite(time_now,
                          register_result.value(),
                          score::common::visitor::logging_serializer::serialize_size(message),
                          [&message](auto data_span) {
                              return score::common::visitor::logging_serializer::serialize(
                                  message, data_span.data(), score::mw::log::detail::GetDataSizeAsLength(data_span));
                          });

    shared_data_.writer_detached.store(true);

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, DefaultParserFactory());

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.new_source_session(0,
                                                 "any app",
                                                 false,
                                                 std::move(session_sender),
                                                 0. /*zero quota KB*/,
                                                 true /*Enable quota enforcement*/,
                                                 2001,
                                                 CreateReaderFactory());

    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session' method and the invoking of the 'checkAndSetQuotaEnforcement' method to test
    // the quota over limitation.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->tick(), false);
    // Call 'tick' again to invoke 'checkAndSetQuotaEnforcement' method after updating total size with the new value.
    EXPECT_EQ(src_session->tick(), false);
}

// To test the True case of the below condition for show_stats() method.
// The condition is:
// if (rate_KBps > quota_KBps_ && quotaEnforcementEnabled_)
TEST_F(DataRouterTestFixture, TestQuotaLimitationWithTimeDurationBiggerThanZeroWithQuotaEnforcementEnabled)
{
    TestMessage message{2345};
    auto type_info = utils::CreateTypeInfo<TestMessage>();
    const auto register_result = writer_.TryRegisterType(type_info);

    const auto time_now = score::mw::log::detail::TimePoint::clock::now();
    writer_.AllocAndWrite(time_now,
                          register_result.value(),
                          score::common::visitor::logging_serializer::serialize_size(message),
                          [&message](auto data_span) {
                              return score::common::visitor::logging_serializer::serialize(
                                  message, data_span.data(), score::mw::log::detail::GetDataSizeAsLength(data_span));
                          });

    shared_data_.writer_detached.store(true);

    auto& logger = score::mw::log::CreateLogger("DRUT", "statistics");

    datarouter::DataRouter router(logger, DefaultParserFactory());

    auto session_sender = score::cpp::pmr::make_unique<daemon::mock::SessionHandleMock>(score::cpp::pmr::get_default_resource());
    auto src_session = router.new_source_session(0,
                                                 "any app",
                                                 false,
                                                 std::move(session_sender),
                                                 0. /*zero quota KB*/,
                                                 true /*Enable quota enforcement*/,
                                                 2001,
                                                 CreateReaderFactory());

    // It's not mandatory to be 10 ms, it's safe and shouldn't cause any flakiness, we only need to guarantee
    // a difference in the time between the instantiation of the 'SourceSession' instance that created
    // by 'new_source_session' method and the invoking of the 'checkAndSetQuotaEnforcement' method to test
    // the quota over limitation.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    EXPECT_NE(src_session, nullptr);
    EXPECT_EQ(src_session->tick(), false);
    // Call 'tick' again to invoke 'checkAndSetQuotaEnforcement' method after updating total size with the new value.
    EXPECT_EQ(src_session->tick(), false);
    // Calling 'show_source_statistics' method to invoke 'show_stats' method.
    router.show_source_statistics(0);
}

// Basic test fixture for DataRouter with synchronized wrappers
class SynchronizedDataRouterTest : public ::testing::Test
{
  protected:
    // Test the synchronized wrapper directly
    void TestSynchronizedWrapper()
    {
        // Create a synchronized wrapper around an int
        synchronized<int> sync_value(42);

        // Test the with_lock method for read access
        int result = sync_value.with_lock([](const auto& value) {
            return value;
        });
        EXPECT_EQ(result, 42);

        // Test the with_lock method for write access
        sync_value.with_lock([](auto& value) {
            value = 100;
        });

        // Verify the value was updated
        result = sync_value.with_lock([](const auto& value) {
            return value;
        });
        EXPECT_EQ(result, 100);
    }

    // Test the LocalSubscriberData synchronization
    void TestLocalSubscriberDataSync()
    {
        synchronized<score::platform::datarouter::LocalSubscriberData> local_data(
            score::platform::datarouter::LocalSubscriberData{});

        local_data.with_lock([](auto& data) {
            data.detach_on_closed_processed = true;
        });

        bool result = local_data.with_lock([](const auto& data) {
            return data.detach_on_closed_processed;
        });

        EXPECT_TRUE(result);
    }

    void TestCommandDataSync()
    {
        synchronized<score::platform::datarouter::CommandData> cmd_data(score::platform::datarouter::CommandData{});

        cmd_data.with_lock([](auto& cmd) {
            cmd.command_detach_on_closed = true;
        });

        bool result = cmd_data.with_lock([](const auto& cmd) {
            return cmd.command_detach_on_closed;
        });

        EXPECT_TRUE(result);
    }

    // Test StatsData synchronization
    void TestStatsDataSync()
    {
        // Direct initialization instead of {}
        synchronized<score::platform::datarouter::StatsData> stats(score::platform::datarouter::StatsData{});

        // Set and update message count
        stats.with_lock([](auto& s) {
            s.message_count = 10;
        });

        stats.with_lock([](auto& s) {
            s.message_count += 5;
        });

        uint64_t result = stats.with_lock([](const auto& s) {
            return s.message_count;
        });

        EXPECT_EQ(result, 15);
    }
};

// Test the synchronized wrapper template
TEST_F(SynchronizedDataRouterTest, TestSynchronizedWrapperTemplate)
{
    TestSynchronizedWrapper();
}

// Test LocalSubscriberData synchronization
TEST_F(SynchronizedDataRouterTest, TestLocalSubscriberDataSynchronization)
{
    TestLocalSubscriberDataSync();
}

// Test CommandData synchronization
TEST_F(SynchronizedDataRouterTest, TestCommandDataSynchronization)
{
    TestCommandDataSync();
}

// Test StatsData synchronization
TEST_F(SynchronizedDataRouterTest, TestStatsDataSynchronization)
{
    TestStatsDataSync();
}

// Test for thread safety with multiple threads accessing a synchronized wrapper
TEST_F(SynchronizedDataRouterTest, TestThreadSafety)
{
    synchronized<int> counter(0);
    const int num_threads = 10;
    const int increments_per_thread = 1000;

    std::vector<std::thread> threads;

    // Create multiple threads that increment the counter
    for (int i = 0; i < num_threads; ++i)
    {
        threads.push_back(std::thread([&counter]() noexcept {
            for (int j = 0; j < increments_per_thread; ++j)
            {
                counter.with_lock([](auto& value) {
                    ++value;
                });
            }
        }));
    }

    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }

    // Verify the counter has been incremented correctly
    int result = counter.with_lock([](auto& value) {
        return value;
    });

    EXPECT_EQ(result, num_threads * increments_per_thread);
}

}  // namespace datarouter
}  // namespace platform
}  // namespace score
