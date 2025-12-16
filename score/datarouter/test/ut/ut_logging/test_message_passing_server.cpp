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

#include "score/concurrency/thread_pool.h"
#include "score/os/mocklib/mock_pthread.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/mw/com/message_passing/message.h"
#include "score/mw/com/message_passing/receiver_mock.h"
#include "score/mw/com/message_passing/sender_mock.h"
#include "score/datarouter/daemon_communication/session_handle_mock.h"

#include "score/optional.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace score::mw::com::message_passing;

namespace score
{
namespace platform
{
namespace internal
{

using ::testing::_;
using ::testing::An;
using ::testing::AnyNumber;
using ::testing::AtMost;
using ::testing::Eq;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::StrictMock;

using score::mw::log::detail::DatarouterMessageIdentifier;
using score::mw::log::detail::ToMessageId;

constexpr pid_t OUR_PID = 4444;

constexpr pid_t CLIENT0_PID = 1000;
constexpr pid_t CLIENT1_PID = 1001;
constexpr pid_t CLIENT2_PID = 1002;

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
        SessionStatus(pid_t pid, score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle)
            : pid_(pid), handle_(std::move(handle))
        {
        }

        void IncrementTickCount()
        {
            std::lock_guard<std::mutex> lock(tick_count_mutex_);
            ++tick_count_;
            tick_count_cond_.notify_all();
        }

        void WaitStartOfFirstTick()
        {
            std::unique_lock<std::mutex> lock(tick_count_mutex_);
            tick_count_cond_.wait(lock, [this]() {
                return tick_count_ != 0;
            });
        }

        pid_t pid_;
        score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle_;

        std::mutex tick_count_mutex_;
        std::condition_variable tick_count_cond_;
        std::uint32_t tick_count_{0};
    };

    void SetUp() override
    {
        using namespace ::score::mw::com;
        message_passing::ReceiverFactory::InjectReceiverMock(&receiver_mock_);
        message_passing::SenderFactory::InjectSenderMock(&sender_mock_);
    }

    void TearDown() override
    {
        using namespace ::score::mw::com;
        message_passing::ReceiverFactory::InjectReceiverMock(nullptr);
        message_passing::SenderFactory::InjectSenderMock(nullptr);
    }

    auto GetCountingSessionFactory()
    {
        return [this](pid_t pid,
                      const score::mw::log::detail::ConnectMessageFromClient& /*conn*/,
                      score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle)
                   -> std::unique_ptr<MessagePassingServer::ISession> {
            std::lock_guard<std::mutex> emplace_lock(map_mutex_);

            auto emplace_result = session_map_.emplace(
                std::piecewise_construct, std::forward_as_tuple(pid), std::forward_as_tuple(pid, std::move(handle)));

            // expect that the pid is unique;
            // this also serves as a test for correct handling of recurring connections with same pid
            EXPECT_TRUE(emplace_result.second);
            SessionStatus& status = emplace_result.first->second;

            ++construct_count_;
            auto session = std::make_unique<MockSession>();
            EXPECT_CALL(*session, tick).Times(AnyNumber()).WillRepeatedly([this, &status]() {
                ++tick_count_;
                status.IncrementTickCount();
                CheckWaitTickUnblock();
                return false;
            });
            EXPECT_CALL(*session, on_acquire_response)
                .Times(AnyNumber())
                .WillRepeatedly([this](const score::mw::log::detail::ReadAcquireResult&) {
                    ++acquire_response_count_;
                });
            EXPECT_CALL(*session, on_closed_by_peer).Times(AtMost(1)).WillOnce([this]() {
                ++closed_by_peer_count_;
            });
            EXPECT_CALL(*session, is_source_closed).Times(AnyNumber()).WillRepeatedly(Return(false));
            EXPECT_CALL(*session, Destruct).Times(1).WillOnce([this, &status]() {
                ++destruct_count_;
                std::lock_guard<std::mutex> erase_lock(map_mutex_);
                session_map_.erase(status.pid_);
                map_cond_.notify_all();
            });
            return session;
        };
    }

    void CheckWaitTickUnblock()
    {
        // atomic fast path, to avoid introduction of explicit thread serialization on tick_blocker_mutex_
        if (!tick_blocker_)
        {
            return;
        }
        std::unique_lock<std::mutex> lock(tick_blocker_mutex_);
        tick_blocker_cond_.wait(lock, [this]() {
            return !tick_blocker_;
        });
    }

    void InstantiateServer(MessagePassingServer::SessionFactory factory = {})
    {
        using namespace ::score::mw::com;

        // capture MessagePassingServer-installed callbacks when provided
        EXPECT_CALL(receiver_mock_,
                    Register(ToMessageId(DatarouterMessageIdentifier::kConnect),
                             (An<message_passing::IReceiver::MediumMessageReceivedCallback>())))
            .WillOnce([this](auto /*id*/, auto callback) {
                connect_callback_ = std::move(callback);
            });
        EXPECT_CALL(receiver_mock_,
                    Register(ToMessageId(DatarouterMessageIdentifier::kAcquireResponse),
                             (An<message_passing::IReceiver::MediumMessageReceivedCallback>())))
            .WillOnce([this](auto /*id*/, auto callback) {
                acquire_response_callback_ = std::move(callback);
            });

        EXPECT_CALL(receiver_mock_, StartListening()).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

        // instantiate MessagePassingServer
        server_.emplace(factory, executor_);
    }

    void UninstantiateServer()
    {
        server_.reset();
    }

    void ExpectOurPidIsQueried()
    {
        EXPECT_CALL(*unistd_mock_, getpid()).WillRepeatedly(Return(OUR_PID));
    }

    void ExpectShortMessageSendInSequence(const DatarouterMessageIdentifier& id, ::testing::Sequence& seq)
    {
        using namespace ::score::mw::com;
        EXPECT_CALL(sender_mock_, Send(An<const message_passing::ShortMessage&>()))
            .InSequence(seq)
            .WillOnce([id](const auto& m) {
                score::cpp::expected_blank<score::os::Error> ret{};
                if (m.pid != OUR_PID || m.id != ToMessageId(id))
                {
                    ret = score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL));
                }
                return ret;
            });
    }

    void ExpectShortMessageSend(const std::uint8_t id)
    {
        using namespace ::score::mw::com;
        EXPECT_CALL(sender_mock_, Send(An<const message_passing::ShortMessage&>())).WillOnce([id](const auto& m) {
            score::cpp::expected_blank<score::os::Error> ret{};
            if (m.pid != OUR_PID || m.id != id)
            {
                ret = score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL));
            }
            return ret;
        });
    }

    void ExpectAndFailShortMessageSend(const DatarouterMessageIdentifier& id)
    {
        using namespace ::score::mw::com;
        EXPECT_CALL(sender_mock_,
                    Send(Matcher<const message_passing::ShortMessage&>(
                        Field(&message_passing::ShortMessage::id, Eq(ToMessageId(id))))))
            .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    }

    StrictMock<::score::mw::com::message_passing::ReceiverMock> receiver_mock_{};
    StrictMock<::score::mw::com::message_passing::SenderMock> sender_mock_{};
    ::score::os::MockGuard<score::os::UnistdMock> unistd_mock_{};
    ::score::concurrency::ThreadPool executor_{2};

    score::cpp::optional<MessagePassingServer> server_;
    score::cpp::callback<void(score::mw::com::message_passing::MediumMessagePayload, pid_t)> connect_callback_;
    score::cpp::callback<void(score::mw::com::message_passing::MediumMessagePayload, pid_t)> acquire_response_callback_;
    score::cpp::callback<void(score::mw::com::message_passing::ShortMessagePayload, pid_t)> release_response_callback_;

    std::mutex map_mutex_;
    std::condition_variable map_cond_;  // currently only used for destruction
    std::unordered_map<pid_t, SessionStatus> session_map_;

    std::int32_t construct_count_{0};
    std::int32_t acquire_response_count_{0};
    std::int32_t release_response_count_{0};
    std::int32_t destruct_count_{0};

    std::mutex tick_blocker_mutex_;
    std::condition_variable tick_blocker_cond_;
    std::atomic<bool> tick_blocker_{false};

    // can be run on a worker thread without explicit synchronization
    std::atomic<std::int32_t> tick_count_{0};
    std::atomic<std::int32_t> closed_by_peer_count_{0};
};

TEST_F(MessagePassingServerFixture, TestNoSession)
{
    InstantiateServer();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestFailedForSettingThreadName)
{
    StrictMock<score::os::MockPthread> pthread_mock_{};
    score::os::Pthread::set_testing_instance(pthread_mock_);
    EXPECT_CALL(pthread_mock_, setname_np(_, _))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    InstantiateServer();
    score::os::Pthread::restore_instance();
    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestFailedStartListening)
{
    using namespace ::score::mw::com;
    MessagePassingServer::SessionFactory factory = {};

    // capture MessagePassingServer-installed callbacks when provided
    EXPECT_CALL(receiver_mock_,
                Register(ToMessageId(DatarouterMessageIdentifier::kConnect),
                         (An<message_passing::IReceiver::MediumMessageReceivedCallback>())))
        .WillOnce([this](auto /*id*/, auto callback) {
            connect_callback_ = std::move(callback);
        });
    EXPECT_CALL(receiver_mock_,
                Register(ToMessageId(DatarouterMessageIdentifier::kAcquireResponse),
                         (An<message_passing::IReceiver::MediumMessageReceivedCallback>())))
        .WillOnce([this](auto /*id*/, auto callback) {
            acquire_response_callback_ = std::move(callback);
        });

    EXPECT_CALL(receiver_mock_, StartListening())
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    // instantiate MessagePassingServer
    server_.emplace(factory, executor_);

    UninstantiateServer();
}

TEST_F(MessagePassingServerFixture, TestOneConnectAcquireRelease)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(tick_count_, 0);
    EXPECT_EQ(construct_count_, 0);
    ::score::mw::com::message_passing::MediumMessagePayload msg_connect{};
    connect_callback_(msg_connect, CLIENT0_PID);
    EXPECT_EQ(construct_count_, 1);

    ::testing::Sequence seq;
    ExpectShortMessageSendInSequence(DatarouterMessageIdentifier::kAcquireRequest, seq);

    session_map_.at(CLIENT0_PID).handle_->AcquireRequest();
    EXPECT_EQ(acquire_response_count_, 0);
    ::score::mw::com::message_passing::MediumMessagePayload msg_acquire{};
    acquire_response_callback_(msg_acquire, CLIENT0_PID);
    EXPECT_EQ(acquire_response_count_, 1);

    EXPECT_EQ(closed_by_peer_count_, 0);
    EXPECT_FALSE(session_map_.empty());

    ExpectAndFailShortMessageSend(DatarouterMessageIdentifier::kAcquireRequest);
    session_map_.at(CLIENT0_PID).handle_->AcquireRequest();
    {
        // let the worker thread process the fault; wait until it erases the client
        std::unique_lock<std::mutex> lock(map_mutex_);
        map_cond_.wait(lock, [this]() {
            return session_map_.empty();
        });
    }

    EXPECT_GE(tick_count_, 1);
    EXPECT_EQ(closed_by_peer_count_, 1);

    EXPECT_EQ(destruct_count_, 1);
    UninstantiateServer();
    EXPECT_EQ(destruct_count_, 1);
}

TEST_F(MessagePassingServerFixture, TestTripleConnectDifferentPids)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(construct_count_, 0);
    ::score::mw::com::message_passing::MediumMessagePayload msg_connect{};
    connect_callback_(msg_connect, CLIENT0_PID);
    connect_callback_(msg_connect, CLIENT1_PID);
    connect_callback_(msg_connect, CLIENT2_PID);
    EXPECT_EQ(construct_count_, 3);

    EXPECT_EQ(closed_by_peer_count_, 0);
    EXPECT_EQ(destruct_count_, 0);

    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count_, 0);
    EXPECT_EQ(destruct_count_, 3);
}

TEST_F(MessagePassingServerFixture, TestTripleConnectSamePid)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(tick_count_, 0);
    EXPECT_EQ(construct_count_, 0);
    ::score::mw::com::message_passing::MediumMessagePayload msg_connect{};
    connect_callback_(msg_connect, CLIENT0_PID);
    connect_callback_(msg_connect, CLIENT0_PID);
    connect_callback_(msg_connect, CLIENT0_PID);
    EXPECT_EQ(construct_count_, 3);

    EXPECT_EQ(closed_by_peer_count_, 2);
    EXPECT_EQ(destruct_count_, 2);
    EXPECT_GE(tick_count_, 2);

    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count_, 2);
    EXPECT_EQ(destruct_count_, 3);
}

TEST_F(MessagePassingServerFixture, TestSamePidWhileRunning)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    tick_blocker_ = true;
    EXPECT_EQ(tick_count_, 0);
    EXPECT_EQ(construct_count_, 0);
    ::score::mw::com::message_passing::MediumMessagePayload msg_connect{};
    connect_callback_(msg_connect, CLIENT0_PID);
    connect_callback_(msg_connect, CLIENT1_PID);
    connect_callback_(msg_connect, CLIENT2_PID);
    EXPECT_EQ(construct_count_, 3);

    // wait until CLIENT0 is blocked inside the first tick
    session_map_.at(CLIENT0_PID).WaitStartOfFirstTick();

    // accumulate other ticks in the queue
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(250ms);

    // we will need to unblock the tick before the callback returns, so start it on a separate thread
    std::thread connect_thread([&]() {
        connect_callback_(msg_connect, CLIENT0_PID);
    });
    EXPECT_EQ(destruct_count_, 0);  // no destruction while we are still in the tick

    tick_blocker_ = false;
    tick_blocker_cond_.notify_all();
    connect_thread.join();
    // now, tick-running CLIENT0 shall have been reconnected

    EXPECT_EQ(closed_by_peer_count_, 1);
    EXPECT_EQ(destruct_count_, 1);
    EXPECT_GE(tick_count_, 2);

    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count_, 1);
    EXPECT_EQ(destruct_count_, 4);
}

TEST_F(MessagePassingServerFixture, TestSamePidWhileQueued)
{
    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    tick_blocker_ = true;
    EXPECT_EQ(tick_count_, 0);
    EXPECT_EQ(construct_count_, 0);
    ::score::mw::com::message_passing::MediumMessagePayload msg_connect{};
    connect_callback_(msg_connect, CLIENT0_PID);
    connect_callback_(msg_connect, CLIENT1_PID);
    connect_callback_(msg_connect, CLIENT2_PID);
    EXPECT_EQ(construct_count_, 3);

    // wait until CLIENT0 is blocked inside the first tick
    session_map_.at(CLIENT0_PID).WaitStartOfFirstTick();

    // accumulate other ticks (CLIENT2 in particular) in the queue
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(250ms);

    // we will need to unblock the tick before the callback returns, so start it on a separate thread
    std::thread connect_thread([&]() {
        connect_callback_(msg_connect, CLIENT2_PID);
    });
    EXPECT_EQ(destruct_count_, 0);  // no destruction while we are still in the tick

    tick_blocker_ = false;
    tick_blocker_cond_.notify_all();
    connect_thread.join();
    // now, tick-queued CLIENT2 shall have been reconnected

    EXPECT_EQ(closed_by_peer_count_, 1);
    EXPECT_EQ(destruct_count_, 1);
    EXPECT_GE(tick_count_, 2);

    UninstantiateServer();

    EXPECT_EQ(closed_by_peer_count_, 1);
    EXPECT_EQ(destruct_count_, 4);
}

TEST_F(MessagePassingServerFixture, TestConnectionTimeoutReached)
{
    ::score::mw::com::message_passing::SenderFactory::InjectSenderMock(&sender_mock_,
                                                                     [](const score::cpp::stop_token& token) noexcept {
                                                                         while (not token.stop_requested())
                                                                         {
                                                                         }
                                                                     });

    ExpectOurPidIsQueried();

    InstantiateServer(GetCountingSessionFactory());

    EXPECT_EQ(tick_count_, 0);
    EXPECT_EQ(construct_count_, 0);
    ::score::mw::com::message_passing::MediumMessagePayload msg_connect{};
    connect_callback_(msg_connect, CLIENT0_PID);
    EXPECT_EQ(construct_count_, 0);

    UninstantiateServer();

    EXPECT_EQ(destruct_count_, 0);
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
    auto session_mock_ptr = session_mock.get();

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
    auto sender = score::cpp::pmr::make_unique<::score::mw::com::message_passing::SenderMock>(score::cpp::pmr::get_default_resource());
    auto sender_raw_ptr = sender.get();
    MessagePassingServer* msg_server = nullptr;

    EXPECT_CALL(*sender_raw_ptr, Send(An<const ::score::mw::com::message_passing::ShortMessage&>())).Times(1);

    MessagePassingServer::SessionHandle session_handle(pid, msg_server, std::move(sender));

    EXPECT_NO_FATAL_FAILURE(session_handle.AcquireRequest());
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
