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

// test_unix_domain_server.cpp
#include "score/os/mocklib/mock_pthread.h"
#include "score/os/mocklib/socketmock.h"
#include "score/os/mocklib/sys_poll_mock.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/os/socket.h"
#include "score/os/unistd.h"
#include "score/os/utils/mocklib/signalmock.h"
#include "unix_domain/unix_domain_server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace score
{
namespace platform
{
namespace internal
{

namespace
{

std::unique_ptr<score::os::Signal> MakeSignalMock()
{
    auto signal = std::make_unique<::testing::NiceMock<score::os::SignalMock>>();
    ON_CALL(*signal, SigEmptySet(::testing::_)).WillByDefault(::testing::Return(0));
    ON_CALL(*signal, SigAddSet(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
    ON_CALL(*signal, PthreadSigMask(::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
    ON_CALL(*signal, SigAction(::testing::_, ::testing::_, ::testing::_)).WillByDefault(::testing::Return(0));
    return signal;
}

}  // namespace

class ISessionMock : public UnixDomainServer::ISession
{
  public:
    ISessionMock(UnixDomainServer::SessionHandle /*h*/) {}

    MOCK_METHOD(bool, Tick, (), (override));
    MOCK_METHOD(void, OnClosedByPeer, (), (override));
    MOCK_METHOD(void, OnCommand, (const std::string&), (override));
};

class UnixDomainServer::SessionWrapper::SessionWrapperTest : public UnixDomainServer::SessionWrapper
{
  public:
    SessionWrapperTest(UnixDomainServer* server, int fd) : SessionWrapper(server, fd) {}

    static void ExpireTimeout(SessionWrapper& wrapper)
    {
        wrapper.timeout_ = TimestampT::clock::now() - std::chrono::milliseconds(1);
    }

    using SessionWrapper::closed_by_peer_;
    using SessionWrapper::enqueued_;
    using SessionWrapper::running_;
    using SessionWrapper::session_;
    using SessionWrapper::session_fd_;
    using SessionWrapper::timeout_;
    using SessionWrapper::to_delete_;
};

class UnixDomainServer::UnixDomainServerTest : public UnixDomainServer
{
  public:
    explicit UnixDomainServerTest(SessionFactory factory = SessionFactory())
        : UnixDomainServer(UnixDomainSockAddr("unix_domain_server_test", true), std::move(factory), MakeSignalMock())
    {
    }

    using UnixDomainServer::ConnectionState;
    using UnixDomainServer::EnqueueTickDirect;
    using UnixDomainServer::ProcessQueue;
    using UnixDomainServer::work_queue_;

    // Expose extracted methods for testing
    using UnixDomainServer::CleanupAllConnections;
    using UnixDomainServer::ProcessActiveConnections;
    using UnixDomainServer::ProcessIdleConnections;
    using UnixDomainServer::ProcessServerIteration;
    using UnixDomainServer::SetupServerSocket;
};

class UnixDomainServerMock : public UnixDomainServer
{
  public:
    UnixDomainServerMock()
        : UnixDomainServer(UnixDomainSockAddr("unix_domain_server_mock", true), SessionFactory(), MakeSignalMock())
    {
    }

    MOCK_METHOD(void, EnqueueTickDirect, (std::int32_t fd), (override));
};

namespace dummy_namespace
{

TEST(TempMarker, TickAlwaysFalse)
{
    TempMarker m;
    EXPECT_FALSE(m.Tick());
    EXPECT_NO_FATAL_FAILURE(m.OnCommand("anything"));
    EXPECT_NO_FATAL_FAILURE(m.OnClosedByPeer());
}

TEST(TempMarker, PolymorphicThroughISession)
{
    std::unique_ptr<UnixDomainServer::ISession> p = std::make_unique<TempMarker>();
    EXPECT_FALSE(p->Tick());
    EXPECT_NO_FATAL_FAILURE(p->OnCommand("cmd"));
    EXPECT_NO_FATAL_FAILURE(p->OnClosedByPeer());
}

class StubISession : public UnixDomainServer::ISession
{
  public:
    bool Tick() override
    {
        return true;
    }
    // no overrides for OnCommand() or OnClosedByPeer()
};

TEST(ISessionTest, ISessionTestSuccessfully)
{
    StubISession m;
    EXPECT_TRUE(m.Tick());
    EXPECT_NO_FATAL_FAILURE(m.OnCommand("anything"));
    EXPECT_NO_FATAL_FAILURE(m.OnClosedByPeer());
}

}  // namespace dummy_namespace

namespace
{

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrEq;

using ServerTest = UnixDomainServer::UnixDomainServerTest;
using WrapperTest = UnixDomainServer::SessionWrapper::SessionWrapperTest;
using RecvResult = score::cpp::expected<ssize_t, score::os::Error>;
using PollResult = score::cpp::expected<std::int32_t, score::os::Error>;
using MessageFlag = score::os::Socket::MessageFlag;

constexpr std::int32_t kServerFd{10};
constexpr std::int32_t kClientFd{20};
constexpr std::int32_t kOtherClientFd{21};
constexpr std::int32_t kPeerPid{1234};
// ProcessServerIteration() skips idle-connection handling for a positive poll timeout.
constexpr std::int32_t kSkipIdle{100};
constexpr std::int32_t kProcessIdle{0};

class CountingSession : public UnixDomainServer::ISession
{
  public:
    explicit CountingSession(UnixDomainServer::SessionHandle /*h*/)
    {
        ++gConstructed;
    }
    bool Tick() override
    {
        ++gTicks;
        return false;
    }
    void OnCommand(const std::string&) override
    {
        ++gCommands;
    }

    static std::atomic<int> gConstructed;
    static std::atomic<int> gCommands;
    static std::atomic<int> gTicks;
};
std::atomic<int> CountingSession::gConstructed{0};
std::atomic<int> CountingSession::gCommands{0};
std::atomic<int> CountingSession::gTicks{0};

class CloseAwareSession : public UnixDomainServer::ISession
{
  public:
    explicit CloseAwareSession(UnixDomainServer::SessionHandle /*h*/) {}

    bool Tick() override
    {
        return false;
    }
    void OnClosedByPeer() override
    {
        ++gPeerClosed;
    }

    static std::atomic<int> gPeerClosed;
};
std::atomic<int> CloseAwareSession::gPeerClosed{0};

class RequeueSession : public UnixDomainServer::ISession
{
  public:
    explicit RequeueSession(UnixDomainServer::SessionHandle /*h*/) {}

    bool Tick() override
    {
        ++gTicks;
        return gTicks.load() == 1;  // true once, false afterwards
    }
    static std::atomic<int> gTicks;
};
std::atomic<int> RequeueSession::gTicks{0};

class EnqueueSession : public UnixDomainServer::ISession
{
  public:
    bool Tick() override
    {
        return true;
    }
};

template <typename Session>
UnixDomainServer::SessionFactory MakeFactory()
{
    return [](const std::string&, UnixDomainServer::SessionHandle h) {
        return std::make_unique<Session>(h);
    };
}

// First recvmsg() of a frame: delivers the message header.
auto DeliverHeader(std::uint16_t payload_len)
{
    return [payload_len](std::int32_t, msghdr* msg, MessageFlag) -> RecvResult {
        SocketMessangerHeader header{};
        header.len = payload_len;
        header.type = MessageType::kDefault;
        header.pid = kPeerPid;
        std::memcpy(msg->msg_iov[0].iov_base, &header, sizeof(header));
        return static_cast<ssize_t>(sizeof(header));
    };
}

// Second recvmsg() of a frame: delivers the null-terminated payload.
auto DeliverPayload(const std::string& payload)
{
    return [payload](std::int32_t, msghdr* msg, MessageFlag) -> RecvResult {
        std::memcpy(msg->msg_iov[0].iov_base, payload.c_str(), payload.size() + 1U);
        return static_cast<ssize_t>(payload.size() + 1U);
    };
}

class UnixDomainServerFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        test_thread_id_ = std::this_thread::get_id();

        score::os::Socket::set_testing_instance(socket_mock_);
        score::os::SysPoll::set_testing_instance(sys_poll_mock_);
        score::os::Unistd::set_testing_instance(unistd_mock_);
        score::os::Pthread::set_testing_instance(pthread_mock_);

        ON_CALL(socket_mock_, socket(_, _, _)).WillByDefault(Return(kServerFd));
        ON_CALL(socket_mock_, bind(_, _, _)).WillByDefault(Return(score::cpp::blank{}));
        ON_CALL(socket_mock_, listen(_, _)).WillByDefault(Return(score::cpp::blank{}));
        ON_CALL(socket_mock_, sendmsg(_, _, _)).WillByDefault(Return(0));
        ON_CALL(sys_poll_mock_, poll(_, _, _)).WillByDefault([this](pollfd* fds, nfds_t count, std::int32_t timeout) {
            return Poll(fds, count, timeout);
        });
        ON_CALL(unistd_mock_, close(_)).WillByDefault(Return(score::cpp::blank{}));
        ON_CALL(unistd_mock_, unlink(_)).WillByDefault(Return(score::cpp::blank{}));
        ON_CALL(pthread_mock_, setname_np(_, _)).WillByDefault(Return(score::cpp::blank{}));
        // Catch-alls (e.g. the server closing its listening fd), overridden by fd-specific expectations.
        EXPECT_CALL(unistd_mock_, close(_)).Times(AnyNumber());
        EXPECT_CALL(unistd_mock_, unlink(_)).Times(AnyNumber());
    }

    void TearDown() override
    {
        ReleaseServerThreads();
        servers_.clear();

        score::os::Socket::restore_instance();
        score::os::SysPoll::restore_instance();
        score::os::Unistd::restore_instance();
        score::os::Pthread::restore_instance();
    }

    // Owned by the fixture so the server threads are released before the servers are joined.
    template <typename Server, typename... Args>
    Server& MakeServer(Args&&... args)
    {
        std::int32_t parked_before{};
        {
            std::lock_guard<std::mutex> lock(server_thread_mutex_);
            parked_before = parked_server_threads_;
        }
        auto server = std::make_unique<Server>(std::forward<Args>(args)...);
        Server& server_ref = *server;
        servers_.push_back(std::move(server));

        std::unique_lock<std::mutex> lock(server_thread_mutex_);
        server_thread_cv_.wait(lock, [this, parked_before]() {
            return parked_server_threads_ > parked_before;
        });
        return server_ref;
    }

    // Lets the servers' own threads run their ServerRoutine() loop freely.
    void ReleaseServerThreads()
    {
        {
            std::lock_guard<std::mutex> lock(server_thread_mutex_);
            server_threads_released_ = true;
        }
        server_thread_cv_.notify_all();
    }

    bool WaitForZeroTimeoutServerPoll()
    {
        std::unique_lock<std::mutex> lock(server_thread_mutex_);
        return server_thread_cv_.wait_for(lock, std::chrono::seconds(2), [this]() {
            return zero_timeout_server_polls_ > 0;
        });
    }

    static ServerTest::ConnectionState MakeConnectionState()
    {
        ServerTest::ConnectionState state{};
        state.connection_pollfd_list.push_back({kServerFd, POLLIN, 0});
        return state;
    }

    void AcceptClient(ServerTest& server, ServerTest::ConnectionState& state, std::int32_t client_fd)
    {
        poll_events_ = {{kServerFd, POLLIN}};
        EXPECT_CALL(socket_mock_, accept(kServerFd, _, _)).WillOnce(Return(client_fd));
        server.ProcessServerIteration(state, kServerFd, kSkipIdle);
    }

    void ReceiveFrame(ServerTest& server,
                      ServerTest::ConnectionState& state,
                      std::int32_t client_fd,
                      const std::string& payload)
    {
        poll_events_ = {{client_fd, POLLIN}};
        EXPECT_CALL(socket_mock_, recvmsg(client_fd, _, MessageFlag::kWaitAll))
            .WillOnce(DeliverHeader(static_cast<std::uint16_t>(payload.size() + 1U)))
            .WillOnce(DeliverPayload(payload));
        server.ProcessServerIteration(state, kServerFd, kSkipIdle);
    }

    NiceMock<score::os::SocketMock> socket_mock_;
    NiceMock<score::os::SysPollMock> sys_poll_mock_;
    NiceMock<score::os::UnistdMock> unistd_mock_;
    NiceMock<score::os::MockPthread> pthread_mock_;

    // fd -> revents reported by the next poll() issued from the test thread.
    std::map<std::int32_t, short> poll_events_;

  private:
    PollResult Poll(pollfd* fds, nfds_t count, std::int32_t timeout)
    {
        if (std::this_thread::get_id() != test_thread_id_)
        {
            // Park the server's own thread so that only the test thread drives the server state.
            std::unique_lock<std::mutex> lock(server_thread_mutex_);
            ++parked_server_threads_;
            zero_timeout_server_polls_ += (timeout == 0) ? 1 : 0;
            server_thread_cv_.notify_all();
            server_thread_cv_.wait(lock, [this]() {
                return server_threads_released_;
            });
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 0;
        }

        std::int32_t ready{0};
        for (nfds_t i = 0U; i < count; ++i)
        {
            const auto event = poll_events_.find(fds[i].fd);
            fds[i].revents = (event != poll_events_.end()) ? event->second : static_cast<short>(0);
            ready += (fds[i].revents != 0) ? 1 : 0;
        }
        return ready;
    }

    std::thread::id test_thread_id_{};
    std::mutex server_thread_mutex_;
    std::condition_variable server_thread_cv_;
    std::int32_t parked_server_threads_{0};
    std::int32_t zero_timeout_server_polls_{0};
    bool server_threads_released_{false};
    std::vector<std::unique_ptr<UnixDomainServer>> servers_;
};

TEST_F(UnixDomainServerFixture, SessionWrapperHandleCommandForwardsToSession)
{
    auto& server_mock = MakeServer<UnixDomainServerMock>();
    constexpr std::int32_t kSessionFd = 10;
    bool tick_called = false;
    std::string last_command{};
    bool closed_by_peer_called = false;
    EXPECT_CALL(server_mock, EnqueueTickDirect(kSessionFd));
    WrapperTest session_test(&server_mock, kSessionFd);

    auto isession_mock = std::make_unique<ISessionMock>(UnixDomainServer::SessionHandle{kSessionFd});
    ISessionMock* raw_isession_mock_ptr = isession_mock.get();
    EXPECT_CALL(*raw_isession_mock_ptr, Tick()).WillRepeatedly([&tick_called]() {
        tick_called = true;
        return false;
    });
    EXPECT_CALL(*raw_isession_mock_ptr, OnClosedByPeer()).WillRepeatedly([&closed_by_peer_called]() {
        closed_by_peer_called = true;
    });
    session_test.session_ = std::move(isession_mock);

    std::string test_command = "test_command";
    EXPECT_CALL(*raw_isession_mock_ptr, OnCommand(test_command))
        .WillRepeatedly([&last_command](const std::string& command) {
            last_command = command;
        });
    EXPECT_TRUE(session_test.HandleCommand(test_command, kPeerPid));
    EXPECT_EQ(last_command, test_command);

    EXPECT_FALSE(session_test.Tick());
    EXPECT_TRUE(tick_called);

    session_test.NotifyClosedByPeer();
    EXPECT_TRUE(closed_by_peer_called);
}

TEST_F(UnixDomainServerFixture, SessionWrapperTryEnqueueForDeleteWhileRunningDoesNotEnqueue)
{
    auto& server_mock = MakeServer<UnixDomainServerMock>();
    EXPECT_CALL(server_mock, EnqueueTickDirect(_)).Times(0);
    WrapperTest wrapper(&server_mock, kClientFd);
    wrapper.session_ = std::make_unique<EnqueueSession>();
    wrapper.running_ = true;
    wrapper.enqueued_ = false;

    EXPECT_TRUE(wrapper.TryEnqueueForDelete(false));
}

TEST_F(UnixDomainServerFixture, SessionWrapperTryEnqueueForDeleteTriggersEnqueue)
{
    auto& server_mock = MakeServer<UnixDomainServerMock>();
    EXPECT_CALL(server_mock, EnqueueTickDirect(kClientFd)).Times(1);
    WrapperTest wrapper(&server_mock, kClientFd);
    wrapper.session_ = std::make_unique<EnqueueSession>();
    wrapper.running_ = false;
    wrapper.enqueued_ = false;

    EXPECT_TRUE(wrapper.TryEnqueueForDelete(true));
    EXPECT_TRUE(wrapper.to_delete_);
    EXPECT_TRUE(wrapper.closed_by_peer_);
    EXPECT_TRUE(wrapper.enqueued_);
}

TEST(UnixDomainSockAddr, NonAbstractRoundTrip)
{
    UnixDomainSockAddr addr("datarouter_socket", false);
    EXPECT_FALSE(addr.IsAbstract());
    const char* s = addr.GetAddressString();
    // since we constructed with isAbstract=false, sun_path[0] ≠ '\0'
    EXPECT_NE(s[0], '\0');
    EXPECT_STREQ(s, addr.addr.sun_path);
}

TEST_F(UnixDomainServerFixture, SessionWrapperBasicFlagsAndTimeout)
{
    auto& server = MakeServer<UnixDomainServer>(
        UnixDomainSockAddr("datarouter_socket", true), UnixDomainServer::SessionFactory(), MakeSignalMock());
    EXPECT_CALL(unistd_mock_, close(7)).Times(1);
    UnixDomainServer::SessionWrapper w(&server, /*fd=*/7);

    EXPECT_FALSE(w.IsMarkedForDelete());
    EXPECT_FALSE(w.GetResetClosedByPeer());

    // without a factory no session is created; handle_command returns now < timeout
    EXPECT_TRUE(w.HandleCommand("", /*peer_pid=*/score::cpp::nullopt));
    EXPECT_TRUE(w.HandleCommand("TT", /*peer_pid=*/score::cpp::nullopt));
    EXPECT_FALSE(w.TryEnqueueForDelete(/*by_peer=*/false));

    w.SetRunning();
    EXPECT_FALSE(w.ResetRunning(/*requeue=*/false));
    EXPECT_TRUE(w.ResetRunning(/*requeue=*/true));
}

TEST_F(UnixDomainServerFixture, SessionHandlePassMessageSendsOnSessionFd)
{
    UnixDomainServer::SessionHandle h(kClientFd);
    EXPECT_CALL(socket_mock_, sendmsg(kClientFd, _, MessageFlag::kNone)).WillOnce(Return(10));

    h.PassMessage("HelloTest");
}

TEST_F(UnixDomainServerFixture, SessionWrapperMoveClosesFdOnlyOnce)
{
    auto& server = MakeServer<ServerTest>();
    EXPECT_CALL(unistd_mock_, close(42)).Times(1);
    UnixDomainServer::SessionWrapper orig(&server, 42);
    EXPECT_FALSE(orig.IsMarkedForDelete());
    EXPECT_FALSE(orig.GetResetClosedByPeer());

    {
        UnixDomainServer::SessionWrapper moved(std::move(orig));
        EXPECT_FALSE(moved.IsMarkedForDelete());
        EXPECT_FALSE(moved.GetResetClosedByPeer());
    }
}

TEST_F(UnixDomainServerFixture, NonAbstractAddressIsUnlinkedBeforeBind)
{
    EXPECT_CALL(unistd_mock_, unlink(StrEq("/tmp/uds_ut_socket"))).Times(1);
    auto& server = MakeServer<UnixDomainServer>(
        UnixDomainSockAddr("/tmp/uds_ut_socket", false), UnixDomainServer::SessionFactory(), MakeSignalMock());

    UnixDomainServer::SessionWrapper w(&server, /*fd*/ 7);
    EXPECT_TRUE(w.HandleCommand("someName", score::cpp::nullopt));
    EXPECT_TRUE(w.HandleCommand("subscriberName", kPeerPid));
}

TEST_F(UnixDomainServerFixture, SetupServerSocketReportsUnlinkFailureAndContinues)
{
    auto& server = MakeServer<ServerTest>();
    UnixDomainSockAddr addr("/tmp/uds_ut_unlink_failure", false);
    EXPECT_CALL(unistd_mock_, unlink(StrEq("/tmp/uds_ut_unlink_failure")))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
    EXPECT_CALL(socket_mock_, bind(kServerFd, _, _));
    EXPECT_CALL(socket_mock_, listen(kServerFd, _));

    ::testing::internal::CaptureStderr();
    const auto server_fd = server.SetupServerSocket(addr);
    const std::string output = ::testing::internal::GetCapturedStderr();

    EXPECT_EQ(server_fd, kServerFd);
    EXPECT_THAT(output, HasSubstr("/tmp/uds_ut_unlink_failure"));
}

TEST_F(UnixDomainServerFixture, AcceptAddsClientConnection)
{
    auto& server = MakeServer<ServerTest>();
    auto state = MakeConnectionState();

    AcceptClient(server, state, kClientFd);

    ASSERT_EQ(state.connection_pollfd_list.size(), 2U);
    EXPECT_EQ(state.connection_pollfd_list[1].fd, kClientFd);
    EXPECT_EQ(state.connection_fd_map.count(kClientFd), 1U);
}

TEST_F(UnixDomainServerFixture, CleanupAllConnectionsClosesClients)
{
    auto& server = MakeServer<ServerTest>();
    auto state = MakeConnectionState();
    AcceptClient(server, state, kClientFd);
    AcceptClient(server, state, kOtherClientFd);
    ASSERT_EQ(state.connection_pollfd_list.size(), 3U);

    EXPECT_CALL(unistd_mock_, close(kClientFd)).Times(1);
    EXPECT_CALL(unistd_mock_, close(kOtherClientFd)).Times(1);
    server.CleanupAllConnections(state);

    ASSERT_EQ(state.connection_pollfd_list.size(), 1U);
    EXPECT_EQ(state.connection_pollfd_list[0].fd, kServerFd);
    EXPECT_TRUE(state.connection_fd_map.empty());
}

TEST_F(UnixDomainServerFixture, FramedMessagesCreateSessionAndForwardCommands)
{
    CountingSession::gConstructed = 0;
    CountingSession::gCommands = 0;
    CountingSession::gTicks = 0;
    auto& server = MakeServer<ServerTest>(MakeFactory<CountingSession>());
    auto state = MakeConnectionState();
    AcceptClient(server, state, kClientFd);

    ReceiveFrame(server, state, kClientFd, "subName");
    ReceiveFrame(server, state, kClientFd, "cmd1");
    ReceiveFrame(server, state, kClientFd, "cmd2");

    EXPECT_EQ(CountingSession::gConstructed.load(), 1);
    EXPECT_EQ(CountingSession::gCommands.load(), 2);
    EXPECT_EQ(CountingSession::gTicks.load(), 3);
    EXPECT_EQ(state.connection_pollfd_list.size(), 2U);
}

TEST_F(UnixDomainServerFixture, IdleClientWithoutSessionIsDroppedAfterTimeout)
{
    auto& server = MakeServer<ServerTest>();
    auto state = MakeConnectionState();
    AcceptClient(server, state, kClientFd);

    poll_events_.clear();
    server.ProcessServerIteration(state, kServerFd, kProcessIdle);
    ASSERT_EQ(state.connection_pollfd_list.size(), 2U) << "dropped within the grace period";

    WrapperTest::ExpireTimeout(state.connection_fd_map.at(kClientFd));
    EXPECT_CALL(unistd_mock_, close(kClientFd)).Times(1);
    server.ProcessServerIteration(state, kServerFd, kProcessIdle);

    EXPECT_EQ(state.connection_pollfd_list.size(), 1U);
    EXPECT_TRUE(state.connection_fd_map.empty());
}

TEST_F(UnixDomainServerFixture, OversizedFrameClosesConnection)
{
    auto& server = MakeServer<ServerTest>();
    auto state = MakeConnectionState();
    AcceptClient(server, state, kClientFd);

    poll_events_ = {{kClientFd, POLLIN}};
    EXPECT_CALL(socket_mock_, recvmsg(kClientFd, _, _)).WillOnce(DeliverHeader(2000U));
    EXPECT_CALL(unistd_mock_, close(kClientFd)).Times(1);
    server.ProcessServerIteration(state, kServerFd, kSkipIdle);

    EXPECT_EQ(state.connection_pollfd_list.size(), 1U);
    EXPECT_TRUE(state.connection_fd_map.empty());
}

TEST_F(UnixDomainServerFixture, TruncatedHeaderClosesConnection)
{
    auto& server = MakeServer<ServerTest>();
    auto state = MakeConnectionState();
    AcceptClient(server, state, kClientFd);

    poll_events_ = {{kClientFd, POLLIN}};
    EXPECT_CALL(socket_mock_, recvmsg(kClientFd, _, _)).WillOnce(Return(RecvResult{3}));
    EXPECT_CALL(unistd_mock_, close(kClientFd)).Times(1);
    server.ProcessServerIteration(state, kServerFd, kSkipIdle);

    EXPECT_EQ(state.connection_pollfd_list.size(), 1U);
    EXPECT_TRUE(state.connection_fd_map.empty());
}

TEST_F(UnixDomainServerFixture, PeerCloseNotifiesSessionAndClosesFd)
{
    CloseAwareSession::gPeerClosed = 0;
    auto& server = MakeServer<ServerTest>(MakeFactory<CloseAwareSession>());
    auto state = MakeConnectionState();
    AcceptClient(server, state, kClientFd);
    ReceiveFrame(server, state, kClientFd, "subscriber");
    ASSERT_EQ(CloseAwareSession::gPeerClosed.load(), 0);

    poll_events_ = {{kClientFd, POLLIN}};
    EXPECT_CALL(socket_mock_, recvmsg(kClientFd, _, _)).WillOnce(Return(RecvResult{0}));
    EXPECT_CALL(unistd_mock_, close(kClientFd)).Times(1);
    server.ProcessServerIteration(state, kServerFd, kSkipIdle);

    EXPECT_EQ(CloseAwareSession::gPeerClosed.load(), 1);
    EXPECT_EQ(state.connection_pollfd_list.size(), 1U);
    EXPECT_TRUE(state.connection_fd_map.empty());
}

TEST_F(UnixDomainServerFixture, TickReturningTrueRequeuesSession)
{
    RequeueSession::gTicks = 0;
    auto& server = MakeServer<ServerTest>(MakeFactory<RequeueSession>());
    auto state = MakeConnectionState();
    AcceptClient(server, state, kClientFd);

    ReceiveFrame(server, state, kClientFd, "subscriber");

    EXPECT_EQ(RequeueSession::gTicks.load(), 2);
    EXPECT_TRUE(server.work_queue_.empty());
}

TEST_F(UnixDomainServerFixture, SessionCreatedWithoutPeerPidIsReported)
{
    CountingSession::gConstructed = 0;
    auto& server = MakeServer<ServerTest>(MakeFactory<CountingSession>());
    WrapperTest wrapper(&server, kClientFd);

    ::testing::internal::CaptureStderr();
    EXPECT_TRUE(wrapper.HandleCommand("subscriber", score::cpp::nullopt));
    const std::string output = ::testing::internal::GetCapturedStderr();

    EXPECT_THAT(output, HasSubstr("Peer PID unavailable"));
    EXPECT_EQ(CountingSession::gConstructed.load(), 1);
}

TEST_F(UnixDomainServerFixture, ServerRoutineResetsPollTimeoutEvery100ms)
{
    MakeServer<ServerTest>();
    ReleaseServerThreads();

    EXPECT_TRUE(WaitForZeroTimeoutServerPoll());
}

TEST_F(UnixDomainServerFixture, ServerIgnoresSetnameError)
{
    EXPECT_CALL(pthread_mock_, setname_np(_, StrEq("server_routine")))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    ::testing::internal::CaptureStderr();
    MakeServer<ServerTest>();
    const std::string output = ::testing::internal::GetCapturedStderr();

    EXPECT_THAT(output, HasSubstr("pthread_setname_np"));
}

using UnixDomainServerDeathTest = UnixDomainServerFixture;

void RunServerBriefly()
{
    UnixDomainServer server(UnixDomainSockAddr("unix_domain_server_death_test", true),
                            UnixDomainServer::SessionFactory(),
                            MakeSignalMock());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Mock setup uses ON_CALL only: the mocked calls happen in the death-test child process.
TEST_F(UnixDomainServerDeathTest, SocketCreationFailureExits)
{
    ON_CALL(socket_mock_, socket(_, _, _))
        .WillByDefault(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EMFILE))));
    EXPECT_DEATH(RunServerBriefly(), "Socket");
}

TEST_F(UnixDomainServerDeathTest, BindFailureExits)
{
    ON_CALL(socket_mock_, bind(_, _, _))
        .WillByDefault(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EADDRINUSE))));
    EXPECT_DEATH(RunServerBriefly(), "bind");
}

TEST_F(UnixDomainServerDeathTest, ListenFailureExits)
{
    ON_CALL(socket_mock_, listen(_, _))
        .WillByDefault(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    EXPECT_DEATH(RunServerBriefly(), "listen");
}

TEST_F(UnixDomainServerDeathTest, PollFailureExits)
{
    ON_CALL(sys_poll_mock_, poll(_, _, _))
        .WillByDefault(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    EXPECT_DEATH(RunServerBriefly(), "poll");
}

TEST_F(UnixDomainServerDeathTest, AcceptFailureExits)
{
    ON_CALL(sys_poll_mock_, poll(_, _, _)).WillByDefault([](pollfd* fds, nfds_t, std::int32_t) -> PollResult {
        fds[0].revents = POLLIN;
        return 1;
    });
    ON_CALL(socket_mock_, accept(_, _, _))
        .WillByDefault(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EMFILE))));
    EXPECT_DEATH(RunServerBriefly(), "accept");
}

TEST(UnixDomainCommon, SetupSignalsFailurePath)
{
    // Given we have our mock gives us the error path on all calls
    std::unique_ptr<::testing::StrictMock<score::os::SignalMock>> signal_mock =
        std::make_unique<::testing::StrictMock<score::os::SignalMock>>();
    auto error = score::os::Error::createUnspecifiedError();

    ::testing::InSequence seq;
    EXPECT_CALL(*signal_mock, SigEmptySet(_)).WillOnce(Return(score::cpp::make_unexpected(error)));
    EXPECT_CALL(*signal_mock, SigAddSet(_, _)).WillOnce(Return(score::cpp::make_unexpected(error)));
    EXPECT_CALL(*signal_mock, PthreadSigMask(_, _)).WillOnce(Return(score::cpp::make_unexpected(error)));
    EXPECT_CALL(*signal_mock, SigAction(_, _, _)).WillOnce(Return(score::cpp::make_unexpected(error)));

    std::unique_ptr<score::os::Signal> s(std::move(signal_mock));

    // When we capture the output
    std::cerr.flush();
    ::testing::internal::CaptureStderr();

    SetupSignals(s);

    std::cerr.flush();
    std::string output = ::testing::internal::GetCapturedStderr();
    std::string expected_err = error.ToString();

    // Then we expect to find the error message four time in there
    unsigned int expected_number_of_error_messages = 4;

    auto count_substrings = [](const std::string& text, const std::string sub_string) -> unsigned int {
        unsigned int occurrences = 0;
        size_t pos = 0;
        while ((pos = text.find(sub_string, pos)) != std::string::npos)
        {
            ++occurrences;
            pos += sub_string.length();
        }
        return occurrences;
    };

    auto number_of_matches = count_substrings(output, expected_err);
    EXPECT_EQ(expected_number_of_error_messages, number_of_matches);
}

TEST(UnixDomainServerExtractedMethods, ProcessIdleConnectionsRemovesOrphanedFd)
{
    // Test orphaned FD cleanup in ProcessIdleConnections
    UnixDomainServer::UnixDomainServerTest::ConnectionState state;

    // Add server FD at index 0 (skipped by loop)
    pollfd server_pfd{};
    server_pfd.fd = 1;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;
    state.connection_pollfd_list.push_back(server_pfd);

    // Add orphaned FD (in poll list but NOT in session map)
    pollfd orphaned_pfd{};
    orphaned_pfd.fd = 42;
    orphaned_pfd.events = POLLIN;
    orphaned_pfd.revents = 0;  // No event - idle
    state.connection_pollfd_list.push_back(orphaned_pfd);

    ASSERT_EQ(state.connection_pollfd_list.size(), 2U);
    ASSERT_EQ(state.connection_fd_map.size(), 0U);

    UnixDomainServer::UnixDomainServerTest::ProcessIdleConnections(state);

    // Orphaned FD removed, only server FD remains
    ASSERT_EQ(state.connection_pollfd_list.size(), 1U);
    ASSERT_FALSE(state.connection_pollfd_list.empty());
    if (!state.connection_pollfd_list.empty())
    {
        EXPECT_EQ(state.connection_pollfd_list[0].fd, 1);
    }
    EXPECT_EQ(state.connection_fd_map.size(), 0U);
}

TEST(UnixDomainServerExtractedMethods, ProcessIdleConnectionsSkipsActiveConnections)
{
    // Test that active connections (with POLLIN) are skipped by ProcessIdleConnections
    UnixDomainServer::UnixDomainServerTest::ConnectionState state;

    pollfd server_pfd{};
    server_pfd.fd = 1;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;
    state.connection_pollfd_list.push_back(server_pfd);

    // Connection with POLLIN set
    pollfd active_pfd{};
    active_pfd.fd = 42;
    active_pfd.events = POLLIN;
    active_pfd.revents = POLLIN;  // Active connection
    state.connection_pollfd_list.push_back(active_pfd);

    ASSERT_EQ(state.connection_pollfd_list.size(), 2U);

    UnixDomainServer::UnixDomainServerTest::ProcessIdleConnections(state);

    // Size unchanged: ProcessIdleConnections() should skip active connections (POLLIN set)
    // and only process idle ones, so both FDs remain in the list
    EXPECT_EQ(state.connection_pollfd_list.size(), 2U);
}

TEST(UnixDomainServerExtractedMethods, ProcessActiveConnectionsRemovesOrphanedFd)
{
    // Test orphaned FD cleanup in ProcessActiveConnections
    UnixDomainServer::UnixDomainServerTest::ConnectionState state;

    pollfd server_pfd{};
    server_pfd.fd = 1;
    server_pfd.events = POLLIN;
    server_pfd.revents = 0;
    state.connection_pollfd_list.push_back(server_pfd);

    pollfd orphaned_pfd{};
    orphaned_pfd.fd = 99;
    orphaned_pfd.events = POLLIN;
    orphaned_pfd.revents = POLLIN;
    state.connection_pollfd_list.push_back(orphaned_pfd);

    ASSERT_EQ(state.connection_pollfd_list.size(), 2U);
    ASSERT_EQ(state.connection_fd_map.size(), 0U);

    UnixDomainServer::UnixDomainServerTest::ProcessActiveConnections(state);

    ASSERT_EQ(state.connection_pollfd_list.size(), 1U);
    EXPECT_EQ(state.connection_pollfd_list[0].fd, 1);
    EXPECT_EQ(state.connection_fd_map.size(), 0U);
}

}  // namespace
}  // namespace internal
}  // namespace platform
}  // namespace score
