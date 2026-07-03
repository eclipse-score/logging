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
#include <map>
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
constexpr std::uint32_t kMaxNotifyBytes{1U};

std::uint32_t gKReceiverQueueMaxSize = 1;

class MockSession : public MessagePassingServer::ISession
{
  public:
    MOCK_METHOD(bool, Tick, (), (override final));
    MOCK_METHOD(void, OnAcquireResponse, (const score::mw::log::detail::ReadAcquireResult&), (override final));
    MOCK_METHOD(void, OnClosedByPeer, (), (override final));
    MOCK_METHOD(bool, IsSourceClosed, (), (override));

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

class MessagePassingServer::MessagePassingServerForTest : public MessagePassingServer
{
  public:
    using MessagePassingServer::connection_timeout_;
    using MessagePassingServer::FinishPreviousSessionWhileLocked;
    using MessagePassingServer::MessagePassingServer;
    using MessagePassingServer::mutex_;
    using MessagePassingServer::pid_session_map_;
    using MessagePassingServer::SessionWrapper;
    using MessagePassingServer::stop_source_;
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

        const score::message_passing::IServerFactory::ServerConfig server_config{gKReceiverQueueMaxSize, 0U, 1U};

        auto server_ptr = score::cpp::pmr::make_unique<testing::StrictMock<score::message_passing::ServerMock>>(
            score::cpp::pmr::get_default_resource());
        server_mock = server_ptr.get();

        EXPECT_CALL(*server_factory_mock,
                    Create(CompareServiceProtocol(
                               ServiceProtocolConfig{"/logging.datarouter_recv", kMaxSendBytes, 0U, kMaxNotifyBytes}),
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
            EXPECT_CALL(*session, Tick).Times(AnyNumber()).WillRepeatedly([this, &status]() {
                ++tick_count;
                status.IncrementTickCount();
                CheckWaitTickUnblock();
                return false;
            });
            EXPECT_CALL(*session, OnAcquireResponse)
                .Times(AnyNumber())
                .WillRepeatedly([this](const score::mw::log::detail::ReadAcquireResult&) {
                    ++acquire_response_count;
                });
            EXPECT_CALL(*session, OnClosedByPeer).Times(AtMost(1)).WillOnce([this]() {
                ++closed_by_peer_count;
            });
            EXPECT_CALL(*session, IsSourceClosed).Times(AnyNumber()).WillRepeatedly(Return(false));
            EXPECT_CALL(*session, Destruct).Times(1).WillOnce([this, &status]() {
                ++destruct_count;
                std::lock_guard<std::mutex> erase_lock(map_mutex);
                session_map.erase(status.pid);
                map_cond.notify_all();
            });
            return session;
        };
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
        server.emplace(factory, server_factory_mock);
    }

    void InstantiateServerWithWatchdog(MessagePassingServer::SessionFactory factory,
                                       MessagePassingServer::AcquireWatchdogConfig watchdog_config)
    {
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

        server.emplace(factory, server_factory_mock, watchdog_config);
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

    StrictMock<::score::message_passing::ServerConnectionMock>* ConnectClientAndSendConnectMessage(const pid_t pid)
    {
        auto connection = std::make_unique<StrictMock<::score::message_passing::ServerConnectionMock>>();
        auto* connection_ptr = connection.get();
        auto emplace_result = connections.emplace(pid, std::move(connection));
        EXPECT_TRUE(emplace_result.second);

        connection_identities.insert_or_assign(pid, score::message_passing::ClientIdentity{pid, 0, 0});
        EXPECT_CALL(*connection_ptr, GetClientIdentity())
            .Times(AnyNumber())
            .WillRepeatedly(ReturnRef(connection_identities.at(pid)));

        // Mirror production behavior: connect_callback runs before any messages.
        std::ignore = connect_callback(*connection_ptr);

        auto message = CreateConnectMessageSample(pid);
        sent_callback(*connection_ptr, message);

        return connection_ptr;
    }

    void UninstantiateServer()
    {
        server.reset();
    }

    void ExpectOurPidIsQueried()
    {
        EXPECT_CALL(*unistd_mock, getpid()).WillRepeatedly(Return(kOurPid));
    }

    void ExpectAcquireNotifyInSequence(const DatarouterMessageIdentifier& id,
                                       ::testing::Sequence& seq,
                                       StrictMock<::score::message_passing::ServerConnectionMock>* connection_mock)
    {
        EXPECT_CALL(*connection_mock, Notify(An<score::cpp::span<const std::uint8_t>>()))
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

    void ExpectAndFailAcquireNotify(StrictMock<::score::message_passing::ServerConnectionMock>* connection_mock)
    {
        EXPECT_CALL(*connection_mock, Notify(Matcher<score::cpp::span<const std::uint8_t>>(_)))
            .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    }

    StrictMock<::score::message_passing::ServerMock>* server_mock{};
    std::shared_ptr<StrictMock<::score::message_passing::ServerFactoryMock>> server_factory_mock;
    ::score::os::MockGuard<score::os::UnistdMock> unistd_mock{};

    score::cpp::optional<MessagePassingServer::MessagePassingServerForTest> server;
    score::message_passing::ConnectCallback connect_callback;
    score::message_passing::DisconnectCallback disconnect_callback;
    score::message_passing::MessageCallback sent_callback;
    score::message_passing::MessageCallback sent_with_reply_callback;

    std::unordered_map<pid_t, std::unique_ptr<StrictMock<::score::message_passing::ServerConnectionMock>>> connections;
    std::map<pid_t, score::message_passing::ClientIdentity> connection_identities;

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
    server.emplace(factory, server_factory_mock);
    ExpectServerDestruction();

    UninstantiateServer();
}

// Covers the StartListening error path: std::cerr << "StartListening: " << ret_listening.error()
TEST_F(MessagePassingServerFixture, TestStartListeningFailure)
{
    MessagePassingServer::SessionFactory factory = {};

    EXPECT_CALL(*server_mock,
                StartListening(Matcher<score::message_passing::ConnectCallback>(_),
                               Matcher<score::message_passing::DisconnectCallback>(_),
                               Matcher<score::message_passing::MessageCallback>(_),
                               Matcher<score::message_passing::MessageCallback>(_)))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    server.emplace(factory, server_factory_mock);
    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestOneConnectAcquireRelease)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(tick_count, 0);
    EXPECT_EQ(construct_count, 0);

    auto* connection_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);

    EXPECT_EQ(construct_count, 1);
    ::testing::Sequence seq;
    ExpectAcquireNotifyInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq, connection_ptr);

    session_map.at(kClienT0Pid).handle->AcquireRequest();
    session_map.at(kClienT0Pid).handle->AcquireRequest();
    EXPECT_EQ(acquire_response_count, 0);

    score::mw::log::detail::ReadAcquireResult acquire_result{0U};
    std::array<std::uint8_t, sizeof(acquire_result) + 1> message{};
    message[0] = score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireResponse);
    std::memcpy(&message[1], &acquire_result, sizeof(acquire_result));

    sent_callback(*connection_ptr, message);

    EXPECT_EQ(acquire_response_count, 1);

    {
        std::array<std::uint8_t, sizeof(score::mw::log::detail::ReadAcquireResult) + 2U> bad_message{};
        bad_message[0] = score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireResponse);
        sent_callback(*connection_ptr, bad_message);
        EXPECT_EQ(acquire_response_count, 2);
    }

    EXPECT_EQ(closed_by_peer_count, 0);
    EXPECT_FALSE(session_map.empty());

    ExpectAndFailAcquireNotify(connection_ptr);
    ExpectServerDestruction();
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

    auto* connection0 = ConnectClientAndSendConnectMessage(kClienT0Pid);
    auto* connection1 = ConnectClientAndSendConnectMessage(kClienT1Pid);
    auto* connection2 = ConnectClientAndSendConnectMessage(kClienT2Pid);
    EXPECT_EQ(construct_count, 3);

    ExpectServerDestruction();
    std::ignore = connection0;
    std::ignore = connection1;
    std::ignore = connection2;

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
    auto* connection0 = ConnectClientAndSendConnectMessage(kClienT0Pid);
    EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
    this->disconnect_callback(connection);
    {
        std::unique_lock<std::mutex> lock(map_mutex);
        ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
            return session_map.find(kClienT0Pid) == session_map.end();
        })) << "Timed out waiting for session cleanup after first disconnect";
    }
    connections.erase(kClienT0Pid);
    auto* connection1 = ConnectClientAndSendConnectMessage(kClienT0Pid);
    EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
    this->disconnect_callback(connection);
    {
        std::unique_lock<std::mutex> lock(map_mutex);
        ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
            return session_map.find(kClienT0Pid) == session_map.end();
        })) << "Timed out waiting for session cleanup after second disconnect";
    }

    connections.erase(kClienT0Pid);
    auto* connection2 = ConnectClientAndSendConnectMessage(kClienT0Pid);
    EXPECT_EQ(construct_count, 3);

    std::ignore = connection0;
    std::ignore = connection1;
    std::ignore = connection2;

    ExpectServerDestruction();

    EXPECT_EQ(closed_by_peer_count, 2);
    EXPECT_EQ(destruct_count, 2);
    EXPECT_GE(tick_count, 2);

    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count, 2);
    EXPECT_EQ(destruct_count, 3);
}

TEST_F(MessagePassingServerFixture, StaleConnectionAcquireResponseShouldBeIgnored)
{
    ExpectOurPidIsQueried();
    InstantiateServer(GetCountingSessionFactory());

    // Connect client (connection0) and create session.
    auto* connection0_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);

    // Disconnect the client and wait until its session is torn down.
    this->disconnect_callback(*connection0_ptr);
    {
        std::unique_lock<std::mutex> lock(map_mutex);
        map_cond.wait(lock, [this]() {
            return session_map.empty();
        });
    }

    // Keep the old connection object alive, but free the PID slot for the new connection.
    auto old_connection = std::move(connections.at(kClienT0Pid));
    connections.erase(kClienT0Pid);
    auto* stale_connection_ptr = old_connection.get();

    // Reconnect client with the same PID (connection1) and create a new session.
    auto* connection1_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);

    score::mw::log::detail::ReadAcquireResult acquire_result{0U};
    std::array<std::uint8_t, sizeof(acquire_result) + 1> response{};
    response[0] = score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireResponse);
    std::memcpy(&response[1], &acquire_result, sizeof(acquire_result));

    // Response on stale connection must be ignored.
    sent_callback(*stale_connection_ptr, response);
    EXPECT_EQ(acquire_response_count, 0);

    // Response on the current connection must be accepted.
    sent_callback(*connection1_ptr, response);
    EXPECT_EQ(acquire_response_count, 1);

    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestSamePidWhileRunning)
{

    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    tick_blocker = true;
    EXPECT_EQ(tick_count, 0);
    EXPECT_EQ(construct_count, 0);
    auto* connection0 = ConnectClientAndSendConnectMessage(kClienT0Pid);
    auto* connection1 = ConnectClientAndSendConnectMessage(kClienT1Pid);
    auto* connection2 = ConnectClientAndSendConnectMessage(kClienT2Pid);
    EXPECT_EQ(construct_count, 3);

    ExpectServerDestruction();

    //  wait until CLIENT0 is blocked inside the first tick
    session_map.at(kClienT0Pid).WaitStartOfFirstTick();

    // we will need to unblock the tick before the callback returns, so start it on a separate thread
    std::thread connect_thread([&]() {
        StrictMock<::score::message_passing::ServerConnectionMock> connection;
        score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
        EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
        this->disconnect_callback(connection);

        // Wait for the old session to be fully destroyed before reconnecting
        {
            std::unique_lock<std::mutex> lock(map_mutex);
            ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
                return session_map.find(kClienT0Pid) == session_map.end();
            })) << "Timed out waiting for old CLIENT0 session destruction";
        }

        connections.erase(kClienT0Pid);

        auto* new_connection = ConnectClientAndSendConnectMessage(kClienT0Pid);
        std::ignore = new_connection;
    });
    EXPECT_EQ(destruct_count, 0);  // no destruction while we are still in the tick

    tick_blocker = false;
    tick_blocker_cond.notify_all();
    connect_thread.join();
    // now, tick-running CLIENT0 shall have been reconnected

    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 1);
    EXPECT_GE(tick_count, 2);

    std::ignore = connection0;
    std::ignore = connection1;
    std::ignore = connection2;
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
    auto* connection0 = ConnectClientAndSendConnectMessage(kClienT0Pid);
    auto* connection1 = ConnectClientAndSendConnectMessage(kClienT1Pid);
    auto* connection2 = ConnectClientAndSendConnectMessage(kClienT2Pid);
    EXPECT_EQ(construct_count, 3);

    ExpectServerDestruction();

    // wait until CLIENT0 is blocked inside the first tick
    session_map.at(kClienT0Pid).WaitStartOfFirstTick();

    // we will need to unblock the tick before the callback returns, so start it on a separate thread
    std::thread connect_thread([&]() {
        StrictMock<::score::message_passing::ServerConnectionMock> connection;
        score::message_passing::ClientIdentity client_identity{kClienT2Pid, 0, 0};
        EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));
        this->disconnect_callback(connection);

        // Wait for the old session to be fully destroyed before reconnecting
        {
            std::unique_lock<std::mutex> lock(map_mutex);
            ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
                return session_map.find(kClienT2Pid) == session_map.end();
            })) << "Timed out waiting for old CLIENT2 session destruction";
        }

        connections.erase(kClienT2Pid);

        auto* new_connection = ConnectClientAndSendConnectMessage(kClienT2Pid);
        std::ignore = new_connection;
    });
    EXPECT_EQ(destruct_count, 0);  // no destruction while we are still in the tick

    tick_blocker = false;
    tick_blocker_cond.notify_all();
    connect_thread.join();
    // now, tick-queued CLIENT2 shall have been reconnected

    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 1);
    EXPECT_GE(tick_count, 2);

    std::ignore = connection0;
    std::ignore = connection1;
    std::ignore = connection2;
    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count, 1);
    EXPECT_EQ(destruct_count, 4);
}

TEST_F(MessagePassingServerFixture, WatchdogShouldTearDownUnresponsiveClient)
{
    ExpectOurPidIsQueried();

    MessagePassingServer::AcquireWatchdogConfig watchdog_config;
    watchdog_config.deadline = std::chrono::milliseconds{0};
    watchdog_config.max_misses = 1U;
    InstantiateServerWithWatchdog(GetCountingSessionFactory(), watchdog_config);

    auto* connection_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);
    ::testing::Sequence seq;
    ExpectAcquireNotifyInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq, connection_ptr);
    session_map.at(kClienT0Pid).handle->AcquireRequest();

    {
        std::unique_lock<std::mutex> lock(map_mutex);
        ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
            return session_map.empty();
        })) << "Timed out waiting for watchdog teardown";
    }

    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, WatchdogMissCountShouldResetOnValidResponse)
{
    ExpectOurPidIsQueried();

    MessagePassingServer::AcquireWatchdogConfig watchdog_config;
    watchdog_config.deadline = std::chrono::milliseconds{0};
    watchdog_config.max_misses = 2U;
    InstantiateServerWithWatchdog(GetCountingSessionFactory(), watchdog_config);

    auto* connection_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);

    ::testing::Sequence seq;
    ExpectAcquireNotifyInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq, connection_ptr);
    ExpectAcquireNotifyInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq, connection_ptr);
    ExpectAcquireNotifyInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq, connection_ptr);

    // 1) First acquire request is missed by the client -> miss_count becomes 1.
    session_map.at(kClienT0Pid).handle->AcquireRequest();
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);

    // 2) Late valid response arrives -> miss_count should reset to 0.
    score::mw::log::detail::ReadAcquireResult acquire_result{0U};
    std::array<std::uint8_t, sizeof(acquire_result) + 1> response{};
    response[0] = score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireResponse);
    std::memcpy(&response[1], &acquire_result, sizeof(acquire_result));
    sent_callback(*connection_ptr, response);

    // 3) One more missed acquire should NOT tear down (max_misses = 2, miss_count should be 1).
    session_map.at(kClienT0Pid).handle->AcquireRequest();
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(session_map.empty());

    // 4) Second miss after the reset should now tear down.
    session_map.at(kClienT0Pid).handle->AcquireRequest();
    {
        std::unique_lock<std::mutex> lock(map_mutex);
        ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
            return session_map.empty();
        })) << "Timed out waiting for watchdog teardown after miss-count reset";
    }

    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, EnobufsFromNotifyShouldNotKillSession)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    auto* connection_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);

    // First Notify() fails with ENOBUFS (notify pool exhausted — transient).
    EXPECT_CALL(*connection_ptr, Notify(Matcher<score::cpp::span<const std::uint8_t>>(_)))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOBUFS))));

    session_map.at(kClienT0Pid).handle->AcquireRequest();

    // Session should still be alive — ENOBUFS is treated as a transient error.
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(session_map.empty());

    // A subsequent successful Notify() should work normally.
    ::testing::Sequence seq;
    ExpectAcquireNotifyInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq, connection_ptr);
    session_map.at(kClienT0Pid).handle->AcquireRequest();

    // Verify session is still alive after successful notify.
    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(session_map.empty());

    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, NonEnobufsNotifyFailureShouldStillKillSession)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    auto* connection_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);

    // Notify() fails with EINVAL (not ENOBUFS) — should still tear down the session.
    ExpectAndFailAcquireNotify(connection_ptr);
    session_map.at(kClienT0Pid).handle->AcquireRequest();

    {
        std::unique_lock<std::mutex> lock(map_mutex);
        ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
            return session_map.empty();
        })) << "Timed out waiting for session teardown after non-ENOBUFS failure";
    }

    ExpectServerDestruction();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, DisconnectDuringNotifyShouldNotCrash)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    auto* connection_ptr = ConnectClientAndSendConnectMessage(kClienT0Pid);

    // Simulate a disconnect arriving right after the Notify() kernel call.
    // In production the disconnect callback fires on the dispatch thread and can
    // race with NotifyAcquireRequest.  NotifyAcquireRequest holds the server mutex
    // for the duration of Notify(), so we spawn the disconnect thread inside the
    // mock (it blocks on the mutex) and join it after AcquireRequest returns.
    std::thread disconnect_thread;
    EXPECT_CALL(*connection_ptr, Notify(Matcher<score::cpp::span<const std::uint8_t>>(_)))
        .WillOnce([this, connection_ptr, &disconnect_thread](const auto /*m*/) {
            disconnect_thread = std::thread([this, connection_ptr]() {
                this->disconnect_callback(*connection_ptr);
            });
            return score::cpp::expected_blank<score::os::Error>{};
        });

    session_map.at(kClienT0Pid).handle->AcquireRequest();
    disconnect_thread.join();

    // Wait for teardown to complete.
    {
        std::unique_lock<std::mutex> lock(map_mutex);
        ASSERT_TRUE(map_cond.wait_for(lock, std::chrono::seconds{1}, [this]() {
            return session_map.empty();
        })) << "Timed out waiting for session teardown after disconnect-during-notify";
    }

    ExpectServerDestruction();
    UninstantiateServer();
}
TEST(MessagePassingServerTests, sessionWrapperCreateTest)
{
    InSequence s;

    auto session_mock = std::make_unique<MockSession>();
    auto* session_mock_ptr = session_mock.get();

    MessagePassingServer::MessagePassingServerForTest::SessionWrapper session_wrapper(
        nullptr, 0, std::move(session_mock));

    EXPECT_FALSE(session_wrapper.IsMarkedForDelete());
    session_wrapper.to_delete = true;
    EXPECT_TRUE(session_wrapper.IsMarkedForDelete());

    session_wrapper.closed_by_peer = true;
    EXPECT_TRUE(session_wrapper.GetResetClosedByPeer());
    EXPECT_FALSE(session_wrapper.GetResetClosedByPeer());

    EXPECT_CALL(*session_mock_ptr, IsSourceClosed).WillOnce(Return(true));
    EXPECT_CALL(*session_mock_ptr, IsSourceClosed).WillOnce(Return(false));
    EXPECT_TRUE(session_wrapper.GetIsSourceClosed());
    EXPECT_FALSE(session_wrapper.GetIsSourceClosed());
}

TEST(MessagePassingServerTests, sessionHandleCreateTest)
{
    const pid_t pid = 0;
    MessagePassingServer* msg_server = nullptr;

    MessagePassingServer::SessionHandle session_handle(pid, msg_server);
    EXPECT_FALSE(session_handle.AcquireRequest());
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

    session_wrapper.enqueued = test_params.input_enqueued;
    session_wrapper.running = test_params.input_running;
    session_wrapper.EnqueueForDeleteWhileLocked(test_params.input_closed_by_peer);
    EXPECT_EQ(session_wrapper.running, test_params.expected_running);
    EXPECT_EQ(session_wrapper.enqueued, test_params.expected_enqueued);
    EXPECT_EQ(session_wrapper.closed_by_peer, test_params.expected_closed_by_peer);
}

TEST(SessionWrapperTest, ResetRunningWhileLocked)
{
    auto session_mock = std::make_unique<MockSession>();

    MessagePassingServer::MessagePassingServerForTest::SessionWrapper session_wrapper(
        nullptr, 0, std::move(session_mock));

    {
        // with enqueued
        session_wrapper.enqueued = false;
        session_wrapper.ResetRunningWhileLocked(true);
        EXPECT_TRUE(session_wrapper.enqueued);
    }

    {
        // without enqueued
        session_wrapper.enqueued = false;
        session_wrapper.ResetRunningWhileLocked(false);
        EXPECT_FALSE(session_wrapper.enqueued);
    }
}

// Covers the connect_callback lambda body by invoking it via connect_callback captured in InstantiateServer.
TEST_F(MessagePassingServerFixture, ConnectCallbackReturnsClientPid)
{
    InstantiateServer(GetCountingSessionFactory());

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
    EXPECT_CALL(connection, GetClientIdentity()).WillOnce(ReturnRef(client_identity));

    // invoke the connect_callback directly — this covers the lambda body
    const auto result = connect_callback(connection);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<std::uintptr_t>(result.value()), static_cast<std::uintptr_t>(kClienT0Pid));

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers the received_send_message_with_reply_callback lambda by invoking it directly.
TEST_F(MessagePassingServerFixture, SentWithReplyCallbackReturnsBlank)
{
    InstantiateServer();

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    std::array<std::uint8_t, 1> msg{0U};
    score::cpp::span<const std::uint8_t> span{msg};

    // invoke the with-reply callback — covers the lambda body (return {})
    EXPECT_NO_FATAL_FAILURE(sent_with_reply_callback(connection, span));

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers MessageCallback empty-message branch (std::cerr + return).
TEST_F(MessagePassingServerFixture, MessageCallbackEmptyMessage)
{
    InstantiateServer(GetCountingSessionFactory());

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
    EXPECT_CALL(connection, GetClientIdentity()).WillRepeatedly(ReturnRef(client_identity));

    // send an empty message — triggers the "message.empty()" branch
    std::array<std::uint8_t, 0> empty_msg{};
    EXPECT_NO_FATAL_FAILURE(sent_callback(connection, empty_msg));

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers MessageCallback kAcquireRequest unsupported branch.
TEST_F(MessagePassingServerFixture, MessageCallbackUnsupportedAcquireRequest)
{
    InstantiateServer(GetCountingSessionFactory());

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
    EXPECT_CALL(connection, GetClientIdentity()).WillRepeatedly(ReturnRef(client_identity));

    std::array<std::uint8_t, 1> msg{score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireRequest)};
    EXPECT_NO_FATAL_FAILURE(sent_callback(connection, msg));

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers MessageCallback default (unknown message type) branch.
TEST_F(MessagePassingServerFixture, MessageCallbackUnknownMessageType)
{
    InstantiateServer(GetCountingSessionFactory());

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
    EXPECT_CALL(connection, GetClientIdentity()).WillRepeatedly(ReturnRef(client_identity));

    // 0xFF is an unknown message type, hitting the default branch
    std::array<std::uint8_t, 1> msg{0xFFU};
    EXPECT_NO_FATAL_FAILURE(sent_callback(connection, msg));

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers SessionHandle::AcquireRequest when sender state is not kReady → return false.
TEST(MessagePassingServerTests, SessionHandleAcquireRequestNotReady)
{
    const pid_t pid = 0;
    MessagePassingServer* msg_server = nullptr;

    MessagePassingServer::SessionHandle session_handle(pid, msg_server);

    const bool result = session_handle.AcquireRequest();
    EXPECT_FALSE(result);
}

// Covers OnConnectRequest "ConnectMessageFromClient too small" branch:
// send a kConnect message whose payload is smaller than sizeof(ConnectMessageFromClient).
TEST_F(MessagePassingServerFixture, OnConnectRequestMessageTooSmall)
{
    InstantiateServer(GetCountingSessionFactory());

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
    EXPECT_CALL(connection, GetClientIdentity()).WillRepeatedly(ReturnRef(client_identity));

    // 1-byte payload: message_type byte only, no ConnectMessageFromClient body
    std::array<std::uint8_t, 1U> too_small_msg{score::cpp::to_underlying(DatarouterMessageIdentifier::kConnect)};
    EXPECT_NO_FATAL_FAILURE(sent_callback(connection, too_small_msg));

    // No session must have been created
    EXPECT_EQ(construct_count, 0);

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers RunWorkerThread connection_timeout_ branch (lines 230-231):
// Set connection_timeout_ to a past time so the worker fires stop_source_.request_stop().
TEST_F(MessagePassingServerFixture, RunWorkerThreadConnectionTimeoutExpired)
{
    InstantiateServer();

    // Access connection_timeout_ via MessagePassingServerForTest
    auto* server_for_test = &(*server);

    {
        std::lock_guard<std::mutex> lock(server_for_test->mutex_);
        // Set to a time in the past — worker will see now >= connection_timeout_ and fire stop
        server_for_test->connection_timeout_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);
    }

    // Give the worker thread time to execute one iteration and hit the branch
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(250ms);

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers FinishPreviousSessionWhileLocked: insert a session into pid_session_map_ directly,
// then call FinishPreviousSessionWhileLocked to exercise the function body.
TEST_F(MessagePassingServerFixture, FinishPreviousSessionWhileLockedCoversBody)
{
    InstantiateServer(GetCountingSessionFactory());

    auto* server_for_test = &(*server);

    // Create a session mock that always returns false from Tick (not requeue)
    auto session_mock = std::make_unique<testing::NiceMock<MockSession>>();
    EXPECT_CALL(*session_mock, Tick).Times(AnyNumber()).WillRepeatedly(Return(false));
    EXPECT_CALL(*session_mock, IsSourceClosed).Times(AnyNumber()).WillRepeatedly(Return(false));
    EXPECT_CALL(*session_mock, OnClosedByPeer).Times(AnyNumber());
    EXPECT_CALL(*session_mock, Destruct).Times(1);

    const pid_t test_pid = 9999;
    {
        std::unique_lock<std::mutex> lock(server_for_test->mutex_);

        // Insert a session directly into the map
        server_for_test->pid_session_map_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(test_pid),
            std::forward_as_tuple(server_for_test, test_pid, std::move(session_mock)));

        auto it = server_for_test->pid_session_map_.find(test_pid);

        // FinishPreviousSessionWhileLocked sets session_finishing_=true and waits on server_cond_.
        // The worker thread will process the enqueued session (to_force_finish=true),
        // extract it from the map, set session_finishing_=false, and notify server_cond_.
        server_for_test->FinishPreviousSessionWhileLocked(it, lock);
        // If we reach here, the function completed successfully.
    }

    ExpectServerDestruction();
    UninstantiateServer();
}

// Covers OnConnectRequest stop_requested() branch:
// Request stop before sending a valid connect message so the early-exit path is taken.
TEST_F(MessagePassingServerFixture, OnConnectRequestStopRequestedCoversEarlyExit)
{
    InstantiateServer(GetCountingSessionFactory());

    // Request stop on the server's stop_source_ before invoking the callback
    auto* server_for_test = &(*server);
    server_for_test->stop_source_.request_stop();

    StrictMock<::score::message_passing::ServerConnectionMock> connection;
    score::message_passing::ClientIdentity client_identity{kClienT0Pid, 0, 0};
    EXPECT_CALL(connection, GetClientIdentity()).Times(AnyNumber()).WillRepeatedly(ReturnRef(client_identity));

    auto message = CreateConnectMessageSample(kClienT0Pid);
    sent_callback(connection, message);

    // The early exit fires before factory_ is called — no session constructed
    EXPECT_EQ(construct_count, 0);

    ExpectServerDestruction();
    UninstantiateServer();
}

}  // namespace internal
}  // namespace platform
}  // namespace score
