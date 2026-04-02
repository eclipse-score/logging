#include "score/datarouter/include/daemon/socketserver.h"

#include "score/concurrency/thread_pool.h"
#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/mock_pthread.h"
#include "score/os/mocklib/socketmock.h"
#include "score/os/mocklib/sys_poll_mock.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/mw/log/configuration/nvconfigfactory.h"
#include "score/mw/log/detail/data_router/data_router_messages.h"
#include "score/mw/log/detail/data_router/shared_memory/reader_factory_mock.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader_mock.h"
#include "score/mw/log/logger.h"
#include "score/datarouter/daemon_communication/session_handle_mock.h"
#include "score/datarouter/datarouter/data_router.h"
#include "score/datarouter/include/daemon/dlt_log_server.h"
#include "score/datarouter/include/logparser/logparser.h"
#include "score/datarouter/src/persistency/i_persistent_dictionary.h"
#include "score/datarouter/src/persistency/stub_persistent_dictionary/stub_persistent_dictionary.h"
#if defined(PERSISTENT_CONFIG_FEATURE_ENABLED)
#include "score/datarouter/src/persistency/ara_per_persistent_dictionary/ara_per_persistent_dictionary.h"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <score/memory.hpp>
#include <cerrno>
#include <cstdlib>
#include <thread>
#include <unordered_map>

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{

class SocketServerThreadNameFixture : public ::testing::Test
{
  protected:
    StrictMock<score::os::MockPthread> pthread_mock_{};

    void SetUp() override
    {
        score::os::Pthread::set_testing_instance(pthread_mock_);
    }

    void TearDown() override
    {
        score::os::Pthread::restore_instance();
    }
};

class InMemoryPersistentDictionary final : public score::platform::datarouter::IPersistentDictionary
{
  public:
    std::string getString(const std::string& key, const std::string& defaultValue) override
    {
        const auto it = string_values_.find(key);
        return it == string_values_.end() ? defaultValue : it->second;
    }

    bool getBool(const std::string& key, const bool defaultValue) override
    {
        const auto it = bool_values_.find(key);
        return it == bool_values_.end() ? defaultValue : it->second;
    }

    void setString(const std::string& key, const std::string& value) override
    {
        string_values_[key] = value;
    }

    void setBool(const std::string& key, const bool value) override
    {
        bool_values_[key] = value;
    }

  private:
    std::unordered_map<std::string, std::string> string_values_{};
    std::unordered_map<std::string, bool> bool_values_{};
};

std::string GetTestDataPath(const std::string& relative_path)
{
    const char* test_srcdir = std::getenv("TEST_SRCDIR");
    const char* test_workspace = std::getenv("TEST_WORKSPACE");
    if (test_srcdir == nullptr || test_workspace == nullptr)
    {
        return relative_path;
    }
    return std::string(test_srcdir) + "/" + test_workspace + "/" + relative_path;
}

}  // namespace

TEST_F(SocketServerThreadNameFixture, SetThreadNameSuccess)
{
    EXPECT_CALL(pthread_mock_, self()).WillOnce(Return(pthread_t{}));
    EXPECT_CALL(pthread_mock_, setname_np(::testing::_, ::testing::StrEq("socketserver")))
        .WillOnce(Return(score::cpp::blank{}));

    score::platform::datarouter::SocketServer::SetThreadName();
}

TEST_F(SocketServerThreadNameFixture, SetThreadNameFailureIsHandled)
{
    EXPECT_CALL(pthread_mock_, self()).WillOnce(Return(pthread_t{}));
    EXPECT_CALL(pthread_mock_, setname_np(::testing::_, ::testing::StrEq("socketserver")))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    score::platform::datarouter::SocketServer::SetThreadName();
}

TEST(SocketServerStatisticsLoggerTest, CreateStatisticsLoggerUsesStatContext)
{
    score::mw::log::Logger& logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();

    EXPECT_EQ(logger.GetContext(), std::string_view{"STAT"});
}

TEST(SocketServerPersistentDictionaryTest, CreatePersistentDictionaryNoAdaptiveReturnsStub)
{
    auto dictionary = score::platform::datarouter::SocketServer::CreatePersistentDictionary(true);

    ASSERT_NE(dictionary, nullptr);
    EXPECT_NE(dynamic_cast<score::platform::datarouter::StubPersistentDictionary*>(dictionary.get()), nullptr);
}

#if !defined(PERSISTENT_CONFIG_FEATURE_ENABLED)
TEST(SocketServerPersistentDictionaryTest, CreatePersistentDictionaryAdaptiveReturnsStub)
{
    auto dictionary = score::platform::datarouter::SocketServer::CreatePersistentDictionary(false);

    ASSERT_NE(dictionary, nullptr);
    EXPECT_NE(dynamic_cast<score::platform::datarouter::StubPersistentDictionary*>(dictionary.get()), nullptr);
}
#endif

TEST(SocketServerPersistentStorageHandlersTest, CreatePersistentStorageHandlersUsesDefaults)
{
    score::platform::datarouter::StubPersistentDictionary dictionary;

    auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);

    EXPECT_TRUE(handlers.is_dlt_enabled);
    const auto config = handlers.load_dlt();
    EXPECT_TRUE(config.channels.empty());
}

TEST(SocketServerPersistentStorageHandlersTest, StoreDltConfigPersistsData)
{
    InMemoryPersistentDictionary dictionary;

    auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);

    score::logging::dltserver::PersistentConfig config{};
    config.filteringEnabled = true;
    config.defaultThreshold = score::mw::log::LogLevel::kWarn;
    config.channels.emplace("CORE", score::logging::dltserver::PersistentConfig::ChannelDescription{});

    handlers.store_dlt(config);

    EXPECT_FALSE(dictionary.getString("dltConfig", "").empty());
}

TEST(SocketServerStaticConfigTest, LoadStaticConfigParsesChannels)
{
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());

    ASSERT_TRUE(config.has_value());
    EXPECT_FALSE(config->channels.empty());
}

TEST(SocketServerDltServerTest, CreateDltServerRespectsEnabledFlag)
{
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);

    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    EXPECT_EQ(dlt_server.GetDltEnabled(), handlers.is_dlt_enabled);
    EXPECT_EQ(dlt_server.getQuotaEnforcementEnabled(), config->quotaEnforcementEnabled);
}

TEST(SocketServerSourceSetupHandlerTest, CreateLogParserFactoryConfiguresParser)
{
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    auto log_parser_factory = score::platform::datarouter::SocketServer::CreateLogParserFactory(dlt_server);

    // Execute the factory to cover the log parser creation
    score::mw::log::NvConfig nv_config({});
    auto parser = log_parser_factory->Create(nv_config);
    EXPECT_NE(parser, nullptr);
}

TEST(SocketServerNvConfigTest, LoadNvConfigReturnsConfig)
{
    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    const auto nv_config = score::platform::datarouter::SocketServer::LoadNvConfig(stats_logger);

    EXPECT_EQ(nv_config.GetDltMsgDesc("nonexistent"), nullptr);
}

TEST(SocketServerNvConfigTest, LoadNvConfigFromFileReturnsDescriptors)
{
    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/class-id.json");

    const auto nv_config = score::platform::datarouter::SocketServer::LoadNvConfig(stats_logger, path);

    EXPECT_NE(nv_config.GetDltMsgDesc("TestMsg"), nullptr);
}

TEST(SocketServerEnableHandlerTest, ConfigureEnableHandlerUpdatesDictionary)
{
    constexpr const char* kOutputEnabledKey = "dltOutputEnabled";
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    InMemoryPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);

    score::platform::datarouter::SocketServer::ConfigureEnableHandler(router, dictionary, dlt_server);

    dlt_server.SetDltOutputEnable(false);
    EXPECT_FALSE(dictionary.getBool(kOutputEnabledKey, true));

    dlt_server.SetDltOutputEnable(true);
    EXPECT_TRUE(dictionary.getBool(kOutputEnabledKey, false));
}

TEST(SocketServerEnableHandlerTest, ConfigureEnableHandlerUpdatesParsers)
{
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    InMemoryPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);
    score::mw::log::NvConfig nv_config = score::mw::log::NvConfigFactory::CreateEmpty();

    auto reader_factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
    auto reader = std::make_unique<score::mw::log::detail::ISharedMemoryReaderMock>();
    EXPECT_CALL(*reader_factory, Create(_, _)).WillOnce(Return(ByMove(std::move(reader))));

    auto handle = score::cpp::pmr::make_unique<score::platform::internal::daemon::mock::SessionHandleMock>(
        score::cpp::pmr::get_default_resource());
    auto session =
        router.new_source_session(10, "APP1", true, std::move(handle), 1.0, false, 1234, std::move(reader_factory));
    ASSERT_NE(session, nullptr);

    score::platform::datarouter::SocketServer::ConfigureEnableHandler(router, dictionary, dlt_server);

    dlt_server.SetDltOutputEnable(false);
    dlt_server.SetDltOutputEnable(true);
}

TEST(SocketServerUnixDomainServerTest, CreateUnixDomainServerReturnsInstance)
{
    score::os::SocketMock socket_mock;
    score::os::SysPollMock sys_poll_mock;

    score::os::Socket::set_testing_instance(socket_mock);
    score::os::SysPoll::set_testing_instance(sys_poll_mock);

    EXPECT_CALL(socket_mock, socket(_, _, _)).WillRepeatedly(Return(30));
    EXPECT_CALL(socket_mock, bind(_, _, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(socket_mock, listen(_, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(sys_poll_mock, poll(_, _, _)).WillRepeatedly([](struct pollfd* in_pollfd, nfds_t, int) {
        in_pollfd[0].revents = 0;
        return 0;
    });

    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    auto server = score::platform::datarouter::SocketServer::CreateUnixDomainServer(dlt_server);
    ASSERT_NE(server, nullptr);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    server.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    score::os::Socket::restore_instance();
    score::os::SysPoll::restore_instance();
}

TEST(SocketServerUnixDomainServerTest, CreateUnixDomainSessionReturnsInstance)
{
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    auto session = score::platform::datarouter::SocketServer::CreateUnixDomainSession(
        dlt_server, "APP1", score::platform::internal::UnixDomainServer::SessionHandle{1});

    EXPECT_NE(session, nullptr);
}

TEST(SocketServerUnixDomainServerTest, CreateUnixDomainSessionFactoryCreatesSession)
{
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    auto factory = score::platform::datarouter::SocketServer::CreateUnixDomainSessionFactory(dlt_server);
    auto session = factory("APP2", score::platform::internal::UnixDomainServer::SessionHandle{2});

    EXPECT_NE(session, nullptr);
}

TEST(SocketServerSharedMemoryFileNameTest, ResolveSharedMemoryFileNameStatic)
{
    score::mw::log::detail::ConnectMessageFromClient conn;
    conn.SetUseDynamicIdentifier(false);
    conn.SetUid(42);
    conn.SetAppId(score::mw::log::detail::LoggingIdentifier{"APP1"});

    const std::string file_name = score::platform::datarouter::SocketServer::ResolveSharedMemoryFileName(conn, "APP1");

    EXPECT_EQ(file_name, "/tmp/logging.APP1.42.shmem");
}

TEST(SocketServerSharedMemoryFileNameTest, ResolveSharedMemoryFileNameDynamic)
{
    score::mw::log::detail::ConnectMessageFromClient conn;
    conn.SetUseDynamicIdentifier(true);
    conn.SetRandomPart({'a', 'b', 'c', 'd', 'e', 'f'});
    conn.SetAppId(score::mw::log::detail::LoggingIdentifier{"APP2"});

    const std::string file_name = score::platform::datarouter::SocketServer::ResolveSharedMemoryFileName(conn, "APP2");

    EXPECT_EQ(file_name, "/tmp/logging-abcdef.shmem");
}

TEST(SocketServerMessagePassingServerTest, CreateMessagePassingServerReturnsInstance)
{
    score::os::FcntlMock fcntl_mock;
    score::os::UnistdMock unistd_mock;

    score::os::Fcntl::set_testing_instance(fcntl_mock);
    score::os::Unistd::set_testing_instance(unistd_mock);

    EXPECT_CALL(fcntl_mock, open(_, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
    EXPECT_CALL(unistd_mock, close(_)).WillRepeatedly(Return(score::cpp::blank{}));

    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);
    score::mw::log::NvConfig nv_config = score::mw::log::NvConfigFactory::CreateEmpty();
    score::concurrency::ThreadPool executor{1};

    auto mp_server =
        score::platform::datarouter::SocketServer::CreateMessagePassingServer(executor, router, dlt_server, nv_config);

    ASSERT_NE(mp_server, nullptr);

    score::os::Fcntl::restore_instance();
    score::os::Unistd::restore_instance();
}

TEST(SocketServerMessagePassingSessionTest, CreateMessagePassingSessionReturnsNullOnOpenFailure)
{
    score::os::FcntlMock fcntl_mock;
    score::os::UnistdMock unistd_mock;

    score::os::Fcntl::set_testing_instance(fcntl_mock);
    score::os::Unistd::set_testing_instance(unistd_mock);

    EXPECT_CALL(fcntl_mock, open(_, _)).WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
    EXPECT_CALL(unistd_mock, close(_)).Times(0);

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);
    score::mw::log::NvConfig nv_config = score::mw::log::NvConfigFactory::CreateEmpty();

    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());
    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    score::mw::log::detail::ConnectMessageFromClient conn;
    conn.SetUseDynamicIdentifier(false);
    conn.SetUid(10);
    conn.SetAppId(score::mw::log::detail::LoggingIdentifier{"APPX"});

    auto reader_factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
    EXPECT_CALL(*reader_factory, Create(_, _)).Times(0);

    auto handle = score::cpp::pmr::make_unique<score::platform::internal::daemon::mock::SessionHandleMock>(
        score::cpp::pmr::get_default_resource());
    auto session = score::platform::datarouter::SocketServer::CreateMessagePassingSession(
        router, dlt_server, nv_config, 123, conn, std::move(handle), std::move(reader_factory));

    EXPECT_EQ(session, nullptr);

    score::os::Fcntl::restore_instance();
    score::os::Unistd::restore_instance();
}

TEST(SocketServerMessagePassingSessionTest, CreateMessagePassingSessionCreatesSourceSession)
{
    score::os::FcntlMock fcntl_mock;
    score::os::UnistdMock unistd_mock;

    score::os::Fcntl::set_testing_instance(fcntl_mock);
    score::os::Unistd::set_testing_instance(unistd_mock);

    EXPECT_CALL(fcntl_mock, open(_, _)).WillOnce(Return(42));
    EXPECT_CALL(unistd_mock, close(42)).WillOnce(Return(score::cpp::blank{}));

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);
    score::mw::log::NvConfig nv_config = score::mw::log::NvConfigFactory::CreateEmpty();

    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());
    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    score::mw::log::detail::ConnectMessageFromClient conn;
    conn.SetUseDynamicIdentifier(false);
    conn.SetUid(10);
    conn.SetAppId(score::mw::log::detail::LoggingIdentifier{"APPX"});

    auto reader_factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
    auto reader = std::make_unique<score::mw::log::detail::ISharedMemoryReaderMock>();
    EXPECT_CALL(*reader_factory, Create(42, 123)).WillOnce(Return(ByMove(std::move(reader))));

    auto handle = score::cpp::pmr::make_unique<score::platform::internal::daemon::mock::SessionHandleMock>(
        score::cpp::pmr::get_default_resource());
    auto session = score::platform::datarouter::SocketServer::CreateMessagePassingSession(
        router, dlt_server, nv_config, 123, conn, std::move(handle), std::move(reader_factory));

    EXPECT_NE(session, nullptr);

    score::os::Fcntl::restore_instance();
    score::os::Unistd::restore_instance();
}

TEST(SocketServerMessagePassingSessionTest, CreateMessagePassingSessionLogsCloseError)
{
    score::os::FcntlMock fcntl_mock;
    score::os::UnistdMock unistd_mock;

    score::os::Fcntl::set_testing_instance(fcntl_mock);
    score::os::Unistd::set_testing_instance(unistd_mock);

    EXPECT_CALL(fcntl_mock, open(_, _)).WillOnce(Return(7));
    EXPECT_CALL(unistd_mock, close(7)).WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO))));

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);
    score::mw::log::NvConfig nv_config = score::mw::log::NvConfigFactory::CreateEmpty();

    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());
    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    score::mw::log::detail::ConnectMessageFromClient conn;
    conn.SetUseDynamicIdentifier(true);
    conn.SetRandomPart({'x', 'y', 'z', '1', '2', '3'});
    conn.SetAppId(score::mw::log::detail::LoggingIdentifier{"APPX"});

    auto reader_factory = std::make_unique<score::mw::log::detail::ReaderFactoryMock>();
    auto reader = std::make_unique<score::mw::log::detail::ISharedMemoryReaderMock>();
    EXPECT_CALL(*reader_factory, Create(7, 77)).WillOnce(Return(ByMove(std::move(reader))));

    auto handle = score::cpp::pmr::make_unique<score::platform::internal::daemon::mock::SessionHandleMock>(
        score::cpp::pmr::get_default_resource());
    auto session = score::platform::datarouter::SocketServer::CreateMessagePassingSession(
        router, dlt_server, nv_config, 77, conn, std::move(handle), std::move(reader_factory));

    EXPECT_NE(session, nullptr);

    score::os::Fcntl::restore_instance();
    score::os::Unistd::restore_instance();
}

TEST(SocketServerMessagePassingServerTest, CreateMessagePassingSessionFactoryUsesDefaults)
{
    score::os::FcntlMock fcntl_mock;
    score::os::UnistdMock unistd_mock;

    score::os::Fcntl::set_testing_instance(fcntl_mock);
    score::os::Unistd::set_testing_instance(unistd_mock);

    EXPECT_CALL(fcntl_mock, open(_, _)).WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
    EXPECT_CALL(unistd_mock, close(_)).Times(0);

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);
    score::mw::log::NvConfig nv_config = score::mw::log::NvConfigFactory::CreateEmpty();

    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());
    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    auto factory =
        score::platform::datarouter::SocketServer::CreateMessagePassingSessionFactory(router, dlt_server, nv_config);

    score::mw::log::detail::ConnectMessageFromClient conn;
    conn.SetUseDynamicIdentifier(false);
    conn.SetUid(5);
    conn.SetAppId(score::mw::log::detail::LoggingIdentifier{"APPA"});

    auto handle = score::cpp::pmr::make_unique<score::platform::internal::daemon::mock::SessionHandleMock>(
        score::cpp::pmr::get_default_resource());
    auto session = factory(5, conn, std::move(handle));

    EXPECT_EQ(session, nullptr);

    score::os::Fcntl::restore_instance();
    score::os::Unistd::restore_instance();
}

TEST(SocketServerEventLoopTest, RunEventLoopExitsWhenRequested)
{
    const std::string path = GetTestDataPath("score/datarouter/test/ut/etc/log-channels.json");
    const auto config = score::platform::datarouter::SocketServer::LoadStaticConfig(path.c_str());
    ASSERT_TRUE(config.has_value());

    score::platform::datarouter::StubPersistentDictionary dictionary;
    const auto handlers = score::platform::datarouter::SocketServer::CreatePersistentStorageHandlers(dictionary);
    auto dlt_server = score::platform::datarouter::SocketServer::CreateDltServer(config.value(), handlers);

    score::mw::log::Logger& stats_logger = score::platform::datarouter::SocketServer::CreateStatisticsLogger();
    score::platform::datarouter::DataRouter router(stats_logger);

    std::atomic_bool exit_requested{false};
    std::thread worker([&]() {
        score::platform::datarouter::SocketServer::RunEventLoop(exit_requested, router, dlt_server, stats_logger);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    exit_requested = true;
    worker.join();
}
