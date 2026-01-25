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

#include "score/datarouter/include/daemon/message_passing_server.h"

#include "score/message_passing/mock/client_connection_mock.h"
#include "score/message_passing/mock/client_factory_mock.h"
#include "score/message_passing/mock/server_connection_mock.h"
#include "score/message_passing/mock/server_factory_mock.h"
#include "score/message_passing/mock/server_mock.h"
#include "score/os/mocklib/mock_pthread.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/datarouter/daemon_communication/session_handle_mock.h"

#include "score/optional.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace score::message_passing;

namespace score
{
namespace platform
{
namespace internal
{

MATCHER_P(CompareServiceProtocol, expected, "")
{
    if (arg.identifier != expected.identifier || arg.max_send_size != expected.max_send_size ||
        arg.max_reply_size != expected.max_reply_size || arg.max_notify_size != expected.max_notify_size)
    {
        return false;
    }
    return true;
}

MATCHER_P(CompareServerConfig, expected, "")
{
    if (arg.max_queued_sends != expected.max_queued_sends ||
        arg.pre_alloc_connections != expected.pre_alloc_connections ||
        arg.max_queued_notifies != expected.max_queued_notifies)
    {
        return false;
    }
    return true;
}

using ::testing::_;
using ::testing::An;
using ::testing::AnyNumber;
using ::testing::AtMost;
using ::testing::InSequence;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrictMock;

using score::mw::log::detail::DatarouterMessageIdentifier;

constexpr pid_t kOurPid = 4444;

constexpr pid_t kClienT0Pid = 1000;
constexpr pid_t kClienT1Pid = 1001;
constexpr pid_t kClienT2Pid = 1002;
constexpr std::uint32_t kMaxSendBytes{17U};

std::uint32_t gKReceiverQueueMaxSize = 0;

class MockSession : public MessagePassingServer::ISession
{
  public:
    MOCK_METHOD(bool, tick, (), (override final));
    MOCK_METHOD(void, on_acquire_response, (const score::mw::log::detail::ReadAcquireResult&), (override final));
    MOCK_METHOD(void, on_closed_by_peer, (), (override final));
    MOCK_METHOD(bool, is_source_closed, (), (override));

    MOCK_METHOD(void, Destruct, (), ());

    ~MockSession() override
    {
        Destruct();
    }
};

class MockIMessagePassingServerSessionWrapper : public IMessagePassingServerSessionWrapper
{
  public:
    MOCK_METHOD(void, EnqueueTickWhileLocked, (pid_t pid), (override));
};

class MessagePassingServerFixture : public ::testing::Test
{
  public:
    struct SessionStatus
    {
        SessionStatus(pid_t process_id,
                      score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> session_handle_ptr)
            : pid(process_id), handle(std::move(session_handle_ptr))
        {
        }

        void IncrementTickCount()
        {
            std::lock_guard<std::mutex> lock(tick_count_mutex);
            ++tick_count;
            tick_count_cond.notify_all();
        }

        void WaitStartOfFirstTick()
        {
            std::unique_lock<std::mutex> lock(tick_count_mutex);
            tick_count_cond.wait(lock, [this]() {
                return tick_count != 0;
            });
        }

        pid_t pid;
        score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle;

        std::mutex tick_count_mutex;
        std::condition_variable tick_count_cond;
        std::uint32_t tick_count{0};
    };

    void SetUp() override
    {
        server_factory_mock = std::make_shared<StrictMock<::score::message_passing::ServerFactoryMock>>();
        client_factory_mock = std::make_shared<StrictMock<::score::message_passing::ClientFactoryMock>>();

        const score::message_passing::IServerFactory::ServerConfig server_config{gKReceiverQueueMaxSize, 0U, 0U};

        auto server_ptr = score::cpp::pmr::make_unique<testing::StrictMock<score::message_passing::ServerMock>>(
            score::cpp::pmr::get_default_resource());
        server_mock = server_ptr.get();

        EXPECT_CALL(
            *server_factory_mock,
            Create(CompareServiceProtocol(ServiceProtocolConfig{"/logging.datarouter_recv", kMaxSendBytes, 0U, 0U}),
                   CompareServerConfig(server_config)))
            .WillOnce(Return(ByMove(std::move(server_ptr))));
    }

    void TearDown() override {}

    auto GetCountingSessionFactory()
    {
        return [this](pid_t pid,
                      const score::mw::log::detail::ConnectMessageFromClient& /*conn*/,
                      score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle)
                   -> std::unique_ptr<MessagePassingServer::ISession> {
            std::lock_guard<std::mutex> emplace_lock(map_mutex);

            auto emplace_result = session_map.emplace(
                std::piecewise_construct, std::forward_as_tuple(pid), std::forward_as_tuple(pid, std::move(handle)));

            // expect that the pid is unique;
            // this also serves as a test for correct handling of recurring connections with same pid
            EXPECT_TRUE(emplace_result.second);
            SessionStatus& status = emplace_result.first->second;

            ++construct_count;
            auto session = std::make_unique<MockSession>();
            EXPECT_CALL(*session, tick).Times(AnyNumber()).WillRepeatedly([this, &status]() {
                ++tick_count;
                status.IncrementTickCount();
                CheckWaitTickUnblock();
                return false;
            });
            EXPECT_CALL(*session, on_acquire_response)
                .Times(AnyNumber())
                .WillRepeatedly([this](const score::mw::log::detail::ReadAcquireResult&) {
                    ++acquire_response_count;
                });
            EXPECT_CALL(*session, on_closed_by_peer).Times(AtMost(1)).WillOnce([this]() {
                ++closed_by_peer_count;
            });
            EXPECT_CALL(*session, is_source_closed).Times(AnyNumber()).WillRepeatedly(Return(false));
            EXPECT_CALL(*session, Destruct).Times(1).WillOnce([this, &status]() {
                ++destruct_count;
                std::lock_guard<std::mutex> erase_lock(map_mutex);
                session_map.erase(status.pid);
                map_cond.notify_all();
            });
            return session;
        };
    }
    void ExpectClientDestruction(StrictMock<::score::message_passing::ClientConnectionMock>* client_mock)
    {
        EXPECT_CALL(*client_mock, Destruct()).Times(AnyNumber());
    }

    void ExpectServerDestruction() const
    {
        EXPECT_CALL(*server_mock, Destruct()).Times(AnyNumber());
    }
    void CheckWaitTickUnblock()
    {
        // atomic fast path, to avoid introduction of explicit thread serialization on tick_blocker_mutex_
        if (!tick_blocker)
        {
            return;
        }
        std::unique_lock<std::mutex> lock(tick_blocker_mutex);
        tick_blocker_cond.wait(lock, [this]() {
            return !tick_blocker;
        });
    }

    void InstantiateServer(MessagePassingServer::SessionFactory factory = {})
    {
        // capture MessagePassingServer-installed callbacks when provided
        EXPECT_CALL(*server_mock,
                    StartListening(Matcher<score::message_passing::ConnectCallback>(_),
                                   Matcher<score::message_passing::DisconnectCallback>(_),
                                   Matcher<score::message_passing::MessageCallback>(_),
                                   Matcher<score::message_passing::MessageCallback>(_)))
            .WillOnce([this](score::message_passing::ConnectCallback con_callback,
                             score::message_passing::DisconnectCallback discon_callback,
                             score::message_passing::MessageCallback sn_callback,
                             score::message_passing::MessageCallback sn_rep_callback) {
                this->connect_callback = std::move(con_callback);
                this->disconnect_callback = std::move(discon_callback);
                this->sent_callback = std::move(sn_callback);
                this->sent_with_reply_callback = std::move(sn_rep_callback);
                return score::cpp::expected_blank<score::os::Error>{};
            });

        // instantiate MessagePassingServer
        server.emplace(factory, server_factory_mock, client_factory_mock);
    }

    auto CreateConnectMessageSample(const pid_t)
    {
        score::mw::log::detail::ConnectMessageFromClient msg;
        score::mw::log::detail::LoggingIdentifier app_id{""};
        msg.SetAppId(app_id);
        msg.SetUid(0U);
        msg.SetUseDynamicIdentifier(false);
        std::array<std::uint8_t, sizeof(msg) + 1> message{};
        message[0] = score::cpp::to_underlying(DatarouterMessageIdentifier::kConnect);
        // NOLINTNEXTLINE(score-banned-function) serialization of trivially copyable
        std::memcpy(&message[1], &msg, sizeof(msg));
        return message;
    }

    StrictMock<::score::message_passing::ClientConnectionMock>* ExpectConnectCallBackCalledAndClientCreated(
        const pid_t pid)
    {
        auto client = score::cpp::pmr::make_unique<testing::StrictMock<score::message_passing::ClientConnectionMock>>(
            score::cpp::pmr::get_default_resource());

        auto* client_mock = client.get();

        EXPECT_CALL(*client_factory_mock,
                    Create(Matcher<const score::message_passing::ServiceProtocolConfig&>(_),
                           Matcher<const score::message_passing::IClientFactory::ClientConfig&>(_)))
            .WillOnce(Return(ByMove(std::move(client))));

        StrictMock<::score::message_passing::ServerConnectionMock> connection;
        score::message_passing::ClientIdentity client_identity{pid, 0, 0};
        EXPECT_CALL(connection, GetClientIdentity()).Times(AnyNumber()).WillRepeatedly(ReturnRef(client_identity));

        auto message = CreateConnectMessageSample(pid);
        sent_callback(connection, message);

        return client_mock;
    }

    void UninstantiateServer()
    {
        server.reset();
    }

    void ExpectOurPidIsQueried()
    {
        EXPECT_CALL(*unistd_mock, getpid()).WillRepeatedly(Return(kOurPid));
    }

    void ExpectMessageSendInSequence(const DatarouterMessageIdentifier& id,
                                     ::testing::Sequence& seq,
                                     StrictMock<::score::message_passing::ClientConnectionMock>* client_mock)
    {
        EXPECT_CALL(*client_mock, Send(An<score::cpp::span<const std::uint8_t>>()))
            .InSequence(seq)
            .WillOnce([id](const auto m) {
                score::cpp::expected_blank<score::os::Error> ret{};
                if (m.front() != score::cpp::to_underlying(id))
                {
                    ret = score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL));
                }
                return ret;
            });
    }

    void ExpectAndFailShortMessageSend(StrictMock<::score::message_passing::ClientConnectionMock>* client_mock)
    {
        EXPECT_CALL(*client_mock, Send(Matcher<score::cpp::span<const std::uint8_t>>(_)))
            .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    }

    StrictMock<::score::message_passing::ServerMock>* server_mock{};
    std::shared_ptr<StrictMock<::score::message_passing::ClientFactoryMock>> client_factory_mock;
    std::shared_ptr<StrictMock<::score::message_passing::ServerFactoryMock>> server_factory_mock;
    ::score::os::MockGuard<score::os::UnistdMock> unistd_mock{};

    score::cpp::optional<MessagePassingServer> server;
    score::message_passing::ConnectCallback connect_callback;
    score::message_passing::DisconnectCallback disconnect_callback;
    score::message_passing::MessageCallback sent_callback;
    score::message_passing::MessageCallback sent_with_reply_callback;

    std::mutex map_mutex;
    std::condition_variable map_cond;  // currently only used for destruction
    std::unordered_map<pid_t, SessionStatus> session_map;

    std::int32_t construct_count{0};
    std::int32_t acquire_response_count{0};
    std::int32_t release_response_count{0};
    std::int32_t destruct_count{0};

    std::mutex tick_blocker_mutex;
    std::condition_variable tick_blocker_cond;
    std::atomic<bool> tick_blocker{false};

    // can be run on a worker thread without explicit synchronization
    std::atomic<std::int32_t> tick_count{0};
    std::atomic<std::int32_t> closed_by_peer_count{0};
};

TEST_F(MessagePassingServerFixture, TestNoSession)
{
    InstantiateServer();
    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestFailedForSettingThreadName)
{
    StrictMock<score::os::MockPthread> pthread_mock{};
    score::os::Pthread::set_testing_instance(pthread_mock);
    EXPECT_CALL(pthread_mock, setname_np(_, _))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    InstantiateServer();
    score::os::Pthread::restore_instance();
    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestFailedStartListening)
{
    MessagePassingServer::SessionFactory factory = {};

    // capture MessagePassingServer-installed callbacks when provided
    EXPECT_CALL(*server_mock,
                StartListening(Matcher<score::message_passing::ConnectCallback>(_),
                               Matcher<score::message_passing::DisconnectCallback>(_),
                               Matcher<score::message_passing::MessageCallback>(_),
                               Matcher<score::message_passing::MessageCallback>(_)))
        .WillOnce([this](score::message_passing::ConnectCallback con_callback,
                         score::message_passing::DisconnectCallback discon_callback,
                         score::message_passing::MessageCallback sn_callback,
                         score::message_passing::MessageCallback sn_rep_callback) {
            this->connect_callback = std::move(con_callback);
            this->disconnect_callback = std::move(discon_callback);
            this->sent_callback = std::move(sn_callback);
            this->sent_with_reply_callback = std::move(sn_rep_callback);
            return score::cpp::expected_blank<score::os::Error>{};
        });
    // instantiate MessagePassingServer
    server.emplace(factory, server_factory_mock, client_factory_mock);
    ExpectServerDestruction();

    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestOneConnectAcquireRelease)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(tick_count, 0);
    EXPECT_EQ(construct_count, 0);

    auto* client = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);

    EXPECT_EQ(construct_count, 1);
    EXPECT_CALL(*client,
                Start(Matcher<score::message_passing::IClientConnection::StateCallback>(_),
                      Matcher<score::message_passing::IClientConnection::NotifyCallback>(_)));

    EXPECT_CALL(*client, GetState()).WillRepeatedly(Return(score::message_passing::IClientConnection::State::kReady));

    ::testing::Sequence seq;
    ExpectMessageSendInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq, client);

    session_map.at(kClienT0Pid).handle->AcquireRequest();
    EXPECT_EQ(acquire_response_count, 0);

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
    EXPECT_CALL(connection, GetClientIdentity()).Times(AnyNumber()).WillRepeatedly(ReturnRef(client_identity));

    score::mw::log::detail::ReadAcquireResult acquire_result{0U};
    std::array<std::uint8_t, sizeof(acquire_result) + 1> message{};
    message[0] = score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireResponse);
    std::memcpy(&message[1], &acquire_result, sizeof(acquire_result));

    sent_callback(connection, message);

    EXPECT_EQ(acquire_response_count, 1);

    EXPECT_EQ(closed_by_peer_count, 0);
    EXPECT_FALSE(session_map.empty());

    ExpectAndFailShortMessageSend(client);
    ExpectServerDestruction();
    ExpectClientDestruction(client);
    session_map.at(kClienT0Pid).handle->AcquireRequest();
    {
        // let the worker thread process the fault; wait until it erases the client
        std::unique_lock<std::mutex> lock(map_mutex);
        map_cond.wait(lock, [this]() {
            return session_map.empty();
        });
    }

    EXPECT_GE(tick_count, 1);
    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 1);
    UninstantiateServer();
    EXPECT_EQ(destruct_count, 1);
}
TEST_F(MessagePassingServerFixture, TestTripleConnectDifferentPids)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(construct_count, 0);

    auto* client0 = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);
    auto* client1 = ExpectConnectCallBackCalledAndClientCreated(kClienT1Pid);
    auto* client2 = ExpectConnectCallBackCalledAndClientCreated(kClienT2Pid);
    EXPECT_EQ(construct_count, 3);

    ExpectServerDestruction();
    ExpectClientDestruction(client0);
    ExpectClientDestruction(client1);
    ExpectClientDestruction(client2);

    EXPECT_EQ(closed_by_peer_count, 0);
    EXPECT_EQ(destruct_count, 0);

    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count, 0);
    EXPECT_EQ(destruct_count, 3);
}

TEST_F(MessagePassingServerFixture, TestTripleConnectSamePid)
{
    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};

    ExpectOurPidIsQueried();
    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(tick_count, 0);
    EXPECT_EQ(construct_count, 0);

    // Recieving new connect with old pid means that old pid owner died and disconnect_callback was called.
    auto* client0 = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);
    EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
    ExpectClientDestruction(client0);
    this->disconnect_callback(connection);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
    auto* client1 = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);
    EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
    ExpectClientDestruction(client1);
    this->disconnect_callback(connection);
    std::this_thread::sleep_for(100ms);

    auto* client2 = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);
    ExpectClientDestruction(client2);
    EXPECT_EQ(construct_count, 3);

    ExpectServerDestruction();

    EXPECT_EQ(closed_by_peer_count, 2);
    EXPECT_EQ(destruct_count, 2);
    EXPECT_GE(tick_count, 2);

    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count, 2);
    EXPECT_EQ(destruct_count, 3);
}

TEST_F(MessagePassingServerFixture, TestSamePidWhileRunning)
{

    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    tick_blocker = true;
    EXPECT_EQ(tick_count, 0);
    EXPECT_EQ(construct_count, 0);
    auto* client0 = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);
    auto* client1 = ExpectConnectCallBackCalledAndClientCreated(kClienT1Pid);
    auto* client2 = ExpectConnectCallBackCalledAndClientCreated(kClienT2Pid);
    EXPECT_EQ(construct_count, 3);

    ExpectServerDestruction();

    // ExpectClientDestruction(client0);
    //  wait until CLIENT0 is blocked inside the first tick
    session_map.at(kClienT0Pid).WaitStartOfFirstTick();

    // accumulate other ticks in the queue
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);

    // we will need to unblock the tick before the callback returns, so start it on a separate thread
    std::thread connect_thread([&]() {
        StrictMock<::score::message_passing::ServerConnectionMock> connection;
        score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
        EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
        ExpectClientDestruction(client0);
        this->disconnect_callback(connection);
        std::this_thread::sleep_for(100ms);

        auto* new_client = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);
        ExpectClientDestruction(new_client);
    });
    EXPECT_EQ(destruct_count, 0);  // no destruction while we are still in the tick

    tick_blocker = false;
    tick_blocker_cond.notify_all();
    connect_thread.join();
    // now, tick-running CLIENT0 shall have been reconnected

    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 1);
    EXPECT_GE(tick_count, 2);

    ExpectClientDestruction(client1);
    ExpectClientDestruction(client2);
    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 4);
}

TEST_F(MessagePassingServerFixture, TestSamePidWhileQueued)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    tick_blocker = true;
    EXPECT_EQ(tick_count, 0);
    EXPECT_EQ(construct_count, 0);
    auto* client0 = ExpectConnectCallBackCalledAndClientCreated(kClienT0Pid);
    auto* client1 = ExpectConnectCallBackCalledAndClientCreated(kClienT1Pid);
    auto* client2 = ExpectConnectCallBackCalledAndClientCreated(kClienT2Pid);
    EXPECT_EQ(construct_count, 3);

    ExpectServerDestruction();

    // wait until CLIENT0 is blocked inside the first tick
    session_map.at(kClienT0Pid).WaitStartOfFirstTick();

    // accumulate other ticks (CLIENT2 in particular) in the queue
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(250ms);

    // we will need to unblock the tick before the callback returns, so start it on a separate thread
    std::thread connect_thread([&]() {
        StrictMock<::score::message_passing::ServerConnectionMock> connection;
        score::message_passing::ClientIdentity client_identity{kClienT2Pid, 0, 0};
        EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
        ExpectClientDestruction(client2);
        this->disconnect_callback(connection);
        std::this_thread::sleep_for(100ms);

        auto* new_client = ExpectConnectCallBackCalledAndClientCreated(kClienT2Pid);
        ExpectClientDestruction(new_client);
    });
    EXPECT_EQ(destruct_count, 0);  // no destruction while we are still in the tick

    tick_blocker = false;
    tick_blocker_cond.notify_all();
    connect_thread.join();
    // now, tick-queued CLIENT2 shall have been reconnected

    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 1);
    EXPECT_GE(tick_count, 2);

    ExpectClientDestruction(client0);
    ExpectClientDestruction(client1);
    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 4);
}

class MessagePassingServer::MessagePassingServerForTest : public MessagePassingServer
{
  public:
    using MessagePassingServer::SessionWrapper;
};

TEST(MessagePassingServerTests, sessionWrapperCreateTest)
{
    InSequence s;

    auto session_mock = std::make_unique<MockSession>();
    auto* session_mock_ptr = session_mock.get();

    MessagePassingServer::MessagePassingServerForTest::SessionWrapper session_wrapper(
        nullptr, 0, std::move(session_mock));

    EXPECT_FALSE(session_wrapper.is_marked_for_delete());
    session_wrapper.to_delete_ = true;
    EXPECT_TRUE(session_wrapper.is_marked_for_delete());

    session_wrapper.closed_by_peer_ = true;
    EXPECT_TRUE(session_wrapper.get_reset_closed_by_peer());
    EXPECT_FALSE(session_wrapper.get_reset_closed_by_peer());

    EXPECT_CALL(*session_mock_ptr, is_source_closed).WillOnce(Return(true));
    EXPECT_CALL(*session_mock_ptr, is_source_closed).WillOnce(Return(false));
    EXPECT_TRUE(session_wrapper.GetIsSourceClosed());
    EXPECT_FALSE(session_wrapper.GetIsSourceClosed());
}

TEST(MessagePassingServerTests, sessionHandleCreateTest)
{
    const pid_t pid = 0;

    auto client = score::cpp::pmr::make_unique<score::message_passing::ClientConnectionMock>(score::cpp::pmr::get_default_resource());

    auto* client_raw_ptr = client.get();
    MessagePassingServer* msg_server = nullptr;

    EXPECT_CALL(*client_raw_ptr,
                Start(Matcher<score::message_passing::IClientConnection::StateCallback>(_),
                      Matcher<score::message_passing::IClientConnection::NotifyCallback>(_)));

    EXPECT_CALL(*client_raw_ptr, GetState())
        .WillRepeatedly(Return(score::message_passing::IClientConnection::State::kReady));

    EXPECT_CALL(*client_raw_ptr, Send(An<score::cpp::span<const std::uint8_t>>())).Times(1);

    MessagePassingServer::SessionHandle session_handle(pid, msg_server, std::move(client));

    EXPECT_NO_FATAL_FAILURE(session_handle.AcquireRequest());
    EXPECT_CALL(*client_raw_ptr, Destruct()).Times(AnyNumber());
}

struct TestParams
{
    const bool input_running;
    const bool input_enqueued;
    const bool input_closed_by_peer;

    const bool expected_running;
    const bool expected_enqueued;
    const bool expected_closed_by_peer;
    const int expected_enqueued_called_count;
};

class SessionWrapperParamTest : public ::testing::TestWithParam<TestParams>
{
  public:
    SessionWrapperParamTest() = default;
};

INSTANTIATE_TEST_CASE_P(SessionWrapperTestEnqueueForDeleteWhileLockedTest,
                        SessionWrapperParamTest,
                        ::testing::Values(
                            // input_closed_by_peer = false, test covers all combinations of running and enqueued
                            TestParams{false, false, false, false, true, false, 1},
                            TestParams{false, true, false, false, true, false, 0},
                            TestParams{true, false, false, true, false, false, 0},
                            TestParams{true, true, false, true, true, false, 0},

                            // input_closed_by_peer = true, test covers all combinations of running and enqueued
                            TestParams{false, false, true, false, true, true, 1},
                            TestParams{false, true, true, false, true, true, 0},
                            TestParams{true, false, true, true, false, true, 0},
                            TestParams{true, true, true, true, true, true, 0}));

TEST_P(SessionWrapperParamTest, EnqueueForDeleteWhileLockedTest)
{
    const auto& test_params = GetParam();

    auto session_mock = std::make_unique<MockSession>();
    MockIMessagePassingServerSessionWrapper server_mock;
    const pid_t pid = 11;

    MessagePassingServer::MessagePassingServerForTest::SessionWrapper session_wrapper(
        &server_mock, pid, std::move(session_mock));

    EXPECT_CALL(server_mock, EnqueueTickWhileLocked(pid)).Times(test_params.expected_enqueued_called_count);

    session_wrapper.enqueued_ = test_params.input_enqueued;
    session_wrapper.running_ = test_params.input_running;
    session_wrapper.enqueue_for_delete_while_locked(test_params.input_closed_by_peer);
    EXPECT_EQ(session_wrapper.running_, test_params.expected_running);
    EXPECT_EQ(session_wrapper.enqueued_, test_params.expected_enqueued);
    EXPECT_EQ(session_wrapper.closed_by_peer_, test_params.expected_closed_by_peer);
}

TEST(SessionWrapperTest, ResetRunningWhileLocked)
{
    auto session_mock = std::make_unique<MockSession>();

    MessagePassingServer::MessagePassingServerForTest::SessionWrapper session_wrapper(
        nullptr, 0, std::move(session_mock));

    {
        // with enqueued
        session_wrapper.enqueued_ = false;
        session_wrapper.reset_running_while_locked(true);
        EXPECT_TRUE(session_wrapper.enqueued_);
    }

    {
        // without enqueued
        session_wrapper.enqueued_ = false;
        session_wrapper.reset_running_while_locked(false);
        EXPECT_FALSE(session_wrapper.enqueued_);
    }
}

}  // namespace internal
}  // namespace platform
}  // namespace score
