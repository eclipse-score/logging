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
#include "score/os/socket.h"
#include "score/os/unistd.h"
#include "score/os/utils/mocklib/signalmock.h"
#include "unix_domain/unix_domain_server.h"

#include <gtest/gtest.h>

namespace score
{
namespace platform
{
namespace internal
{

class ISessionMock : public UnixDomainServer::ISession
{
  public:
    ISessionMock(UnixDomainServer::SessionHandle /*h*/) {}

    MOCK_METHOD(bool, tick, (), (override));
    MOCK_METHOD(void, on_closed_by_peer, (), (override));
    MOCK_METHOD(void, on_command, (const std::string&), (override));
};

class UnixDomainServer::SessionWrapper::SessionWrapperTest : public UnixDomainServer::SessionWrapper
{
  public:
    SessionWrapperTest(UnixDomainServer* server, int fd) : SessionWrapper(server, fd) {}

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
    UnixDomainServerTest()
        : UnixDomainServer(UnixDomainSockAddr("test_" + std::to_string(getpid()) + "_" + std::to_string(rand()), true),
                           SessionFactory())
    {
    }

    using UnixDomainServer::ConnectionState;
    using UnixDomainServer::enqueue_tick_direct;
    using UnixDomainServer::process_queue;
    using UnixDomainServer::work_queue_;

    // Expose extracted methods for testing
    using UnixDomainServer::cleanup_all_connections;
    using UnixDomainServer::process_active_connections;
    using UnixDomainServer::process_idle_connections;
    using UnixDomainServer::process_server_iteration;
    using UnixDomainServer::setup_server_socket;
};

class UnixDomainServerMock : public UnixDomainServer
{
  public:
    UnixDomainServerMock()
        : UnixDomainServer(UnixDomainSockAddr("mock_" + std::to_string(getpid()) + "_" + std::to_string(rand()), true),
                           SessionFactory())
    {
    }

    MOCK_METHOD(void, enqueue_tick_direct, (std::int32_t fd), (override));
};

namespace dummy_namespace
{

TEST(TempMarker, TickAlwaysFalse)
{
    temp_marker m;
    EXPECT_FALSE(m.tick());
    EXPECT_NO_FATAL_FAILURE(m.on_command("anything"));
    EXPECT_NO_FATAL_FAILURE(m.on_closed_by_peer());
}

TEST(TempMarker, PolymorphicThroughISession)
{
    std::unique_ptr<UnixDomainServer::ISession> p = std::make_unique<temp_marker>();
    EXPECT_FALSE(p->tick());
    EXPECT_NO_FATAL_FAILURE(p->on_command("cmd"));
    EXPECT_NO_FATAL_FAILURE(p->on_closed_by_peer());
}

class StubISession : public UnixDomainServer::ISession
{
  public:
    bool tick() override
    {
        return true;
    }
    // no overrides for on_command() or on_closed_by_peer()
};

TEST(ISessionTest, ISessionTestSuccessfully)
{
    StubISession m;
    EXPECT_TRUE(m.tick());
    EXPECT_NO_FATAL_FAILURE(m.on_command("anything"));
    EXPECT_NO_FATAL_FAILURE(m.on_closed_by_peer());
}

TEST(SessionWrapperTest, HandleCommand)
{
    UnixDomainServerMock server_mock;
    constexpr std::uint8_t server_fd = 10U;
    constexpr std::uint16_t peer_pid = 1234U;
    bool tick_called = false;
    std::string last_command{};
    bool closed_by_peer_called = false;
    EXPECT_CALL(server_mock, enqueue_tick_direct(server_fd));
    UnixDomainServer::SessionWrapper::SessionWrapperTest session_test(&server_mock, server_fd);

    auto isession_mock = std::make_unique<ISessionMock>(UnixDomainServer::SessionHandle{server_fd});
    ISessionMock* raw_isession_mock_ptr = isession_mock.get();
    EXPECT_CALL(*raw_isession_mock_ptr, tick()).WillRepeatedly([&tick_called]() {
        tick_called = true;
        return false;
    });
    EXPECT_CALL(*raw_isession_mock_ptr, on_closed_by_peer()).WillRepeatedly([&closed_by_peer_called]() {
        closed_by_peer_called = true;
    });
    session_test.session_ = std::move(isession_mock);

    std::string test_command = "test_command";
    EXPECT_CALL(*raw_isession_mock_ptr, on_command(test_command))
        .WillRepeatedly([&last_command](const std::string& command) {
            last_command = command;
        });
    bool result = session_test.handle_command(test_command, peer_pid);
    EXPECT_TRUE(result);
    EXPECT_EQ(last_command, test_command);

    bool tick_result = session_test.tick();
    EXPECT_FALSE(tick_result);
    EXPECT_TRUE(tick_called);

    session_test.notify_closed_by_peer();
    EXPECT_TRUE(closed_by_peer_called);
}

class EnqueueSession : public UnixDomainServer::ISession
{
  public:
    bool tick() override
    {
        return true;
    }
};

TEST(SessionWrapperTest, TryEnqueueForDeleteWithSessionAlreadyRunningNoEnqueue)
{
    UnixDomainServerMock server_mock;
    constexpr std::uint8_t server_fd = 10U;
    UnixDomainServer::SessionWrapper::SessionWrapperTest wrapper(&server_mock, server_fd);
    wrapper.session_ = std::make_unique<EnqueueSession>();
    wrapper.running_ = true;
    wrapper.enqueued_ = false;

    bool result = wrapper.try_enqueue_for_delete(false);
    EXPECT_TRUE(result);
}

TEST(SessionWrapperTest, TryEnqueueForDeleteWithSessionTriggersEnqueue)
{
    UnixDomainServerMock server_mock;
    constexpr std::uint8_t server_fd = 42U;
    uint8_t enqueue_call_count = 0U;
    std::int32_t last_fd = 0U;
    EXPECT_CALL(server_mock, enqueue_tick_direct(server_fd))
        .WillRepeatedly([&enqueue_call_count, &last_fd](std::int32_t fd) {
            ++enqueue_call_count;
            last_fd = fd;
        });
    UnixDomainServer::SessionWrapper::SessionWrapperTest wrapper(&server_mock, server_fd);
    wrapper.session_ = std::make_unique<EnqueueSession>();
    wrapper.running_ = false;
    wrapper.enqueued_ = false;

    bool result = wrapper.try_enqueue_for_delete(true);
    EXPECT_TRUE(result);
    EXPECT_EQ(enqueue_call_count, 1);
    EXPECT_EQ(last_fd, server_fd);
    EXPECT_TRUE(wrapper.to_delete_);
    EXPECT_TRUE(wrapper.closed_by_peer_);
    EXPECT_TRUE(wrapper.enqueued_);
}

}  // namespace dummy_namespace

namespace
{

// we will reuse your temp_marker
using dummy_namespace::temp_marker;

// a tiny factory that always returns a temp_marker
static const UnixDomainServer::SessionFactory kFactory = [](const std::string&, UnixDomainServer::SessionHandle) {
    return std::make_unique<temp_marker>();
};

class CountingSession : public UnixDomainServer::ISession
{
  public:
    explicit CountingSession(UnixDomainServer::SessionHandle /*h*/)
    {
        ++constructed;
    }
    bool tick() override
    {
        ++ticks;
        return false;
    }
    void on_command(const std::string&) override
    {
        ++commands;
    }

    static std::atomic<int> constructed;
    static std::atomic<int> commands;
    static std::atomic<int> ticks;
};
std::atomic<int> CountingSession::constructed{0};
std::atomic<int> CountingSession::commands{0};
std::atomic<int> CountingSession::ticks{0};

static UnixDomainSockAddr make_temp_addr_abstract_false()
{
    std::string name = "/tmp/uds_ut_" + std::to_string(getpid()) + "_" + std::to_string(rand() & 0xffff);
    return UnixDomainSockAddr(name, /*isAbstract=*/false);
}

static UnixDomainSockAddr make_temp_addr_abstract_false(std::string& name)
{
    name = "/tmp/uds_ut_" + std::to_string(getpid()) + "_" + std::to_string(rand() & 0xffff);
    return UnixDomainSockAddr(name, /*isAbstract=*/false);
}

TEST(UnixDomainSockAddr, NonAbstractRoundTrip)
{
    UnixDomainSockAddr addr("datarouter_socket", false);
    EXPECT_FALSE(addr.is_abstract());
    const char* s = addr.get_address_string();
    // since we constructed with isAbstract=false, sun_path[0] ≠ '\0'
    EXPECT_NE(s[0], '\0');
    EXPECT_STREQ(s, addr.addr_.sun_path);
}

TEST(UnixDomainServerSessionWrapper, BasicFlagsAndTimeout)
{
    UnixDomainSockAddr addr("datarouter_socket", true);
    UnixDomainServer server(addr, UnixDomainServer::SessionFactory());
    UnixDomainServer::SessionWrapper w(&server, /*fd=*/7);

    EXPECT_FALSE(w.is_marked_for_delete());
    EXPECT_FALSE(w.get_reset_closed_by_peer());

    // before a real session_ is constructed, handle_command("") returns now<timeout
    EXPECT_TRUE(w.handle_command("", /*peer_pid=*/score::cpp::nullopt));
    EXPECT_TRUE(w.handle_command("TT", /*peer_pid=*/score::cpp::nullopt));
    EXPECT_FALSE(w.try_enqueue_for_delete(/*by_peer=*/false));

    w.set_running();
    EXPECT_FALSE(w.reset_running(/*requeue=*/false));
    EXPECT_TRUE(w.reset_running(/*requeue=*/true));
}

TEST(SessionHandle, PassMessage)
{
    int fake_fd = 42;
    UnixDomainSockAddr addr("datarouter_socket", true);
    UnixDomainServer server(addr, UnixDomainServer::SessionFactory());

    UnixDomainServer::SessionHandle h(fake_fd);
    EXPECT_NO_FATAL_FAILURE(h.pass_message("HelloTest"));
}

TEST(SessionWrapperMove, CanMoveAndDestructWithoutCrash)
{
    UnixDomainSockAddr addr("datarouter_socket", true);
    UnixDomainServer server(addr, {});
    UnixDomainServer::SessionWrapper orig(&server, 42);
    EXPECT_FALSE(orig.is_marked_for_delete());
    EXPECT_FALSE(orig.get_reset_closed_by_peer());

    EXPECT_NO_FATAL_FAILURE({
        UnixDomainServer::SessionWrapper moved(std::move(orig));
        EXPECT_FALSE(moved.is_marked_for_delete());
        EXPECT_FALSE(moved.get_reset_closed_by_peer());
    });
}

TEST(UnixDomainServerSessionWrapper, ServerSessionWrapperWithRealFactory)
{
    UnixDomainSockAddr addr = make_temp_addr_abstract_false();

    UnixDomainServer server(addr, {});

    UnixDomainServer::SessionWrapper w(&server, /*fd*/ 7);

    EXPECT_FALSE(w.is_marked_for_delete());
    EXPECT_FALSE(w.get_reset_closed_by_peer());

    EXPECT_TRUE(w.handle_command("someName", score::cpp::nullopt));
    EXPECT_TRUE(w.handle_command("someName", score::cpp::nullopt));
    EXPECT_TRUE(w.handle_command("subscriberName", /*peer_pid=*/static_cast<int>(::getpid())));
}

TEST(UnixDomainServerAcceptTest, AcceptsOneClientConnection)
{
    std::string path_ = "/tmp/uds_accept_test_" + std::to_string(getpid());
    ::unlink(path_.c_str());

    UnixDomainSockAddr addr_(path_, /* isAbstract = */ false);
    UnixDomainServer* server_ = new UnixDomainServer(addr_, kFactory);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // create a client socket
    int client = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(client, 0) << strerror(errno);

    struct sockaddr_un sun{};
    sun.sun_family = AF_UNIX;
    strncpy(sun.sun_path, path_.c_str(), sizeof(sun.sun_path) - 1);

    ASSERT_EQ(0, ::connect(client, reinterpret_cast<struct sockaddr*>(&sun), sizeof(sun))) << strerror(errno);

    // now wait a bit so that server_routine's accept() branch fires
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // if we reach here, accept() did not abort
    EXPECT_TRUE(::close(client) == 0);

    delete server_;
}

TEST(UnixDomainServerCleanup, DestructorProcessesPendingConnections)
{
    std::string path;
    auto addr = make_temp_addr_abstract_false(path);  // non‑abstract file‑system socket

    // Scope the server so its destructor will run cleanup
    {
        UnixDomainServer server(addr, kFactory);

        // give server a moment to bind & start listening
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // open two real UNIX‐domain clients and connect them
        for (int i = 0; i < 2; ++i)
        {
            int cfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
            ASSERT_GE(cfd, 0) << strerror(errno);

            sockaddr_un su{};
            su.sun_family = AF_UNIX;
            // sun_path must be null‑terminated and include the path
            std::strncpy(su.sun_path, path.c_str(), sizeof(su.sun_path) - 1);

            int rc = ::connect(cfd, reinterpret_cast<sockaddr*>(&su), sizeof(sockaddr_un));
            ASSERT_EQ(rc, 0) << strerror(errno);

            // leave cfd open; server will accept it
        }

        // give server time to wake up, accept both
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    SUCCEED();
}

TEST(UnixDomainServerHandleCmd, AllBranchesViaFramedMessages)
{
    CountingSession::constructed = 0;
    CountingSession::commands = 0;
    CountingSession::ticks = 0;

    std::string path;
    auto addr = make_temp_addr_abstract_false(path);  // non‑abstract file‑system socket
    ::unlink(path.c_str());

    UnixDomainServer::SessionFactory factory = [](const std::string&, UnixDomainServer::SessionHandle h) {
        return std::make_unique<CountingSession>(std::move(h));
    };

    UnixDomainServer server(addr, factory);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // bind & listen

    // open one client and connect
    int cfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(cfd, 0) << strerror(errno);

    sockaddr_un su{};
    su.sun_family = AF_UNIX;
    std::strncpy(su.sun_path, path.c_str(), sizeof(su.sun_path) - 1);

    ASSERT_EQ(::connect(cfd, reinterpret_cast<sockaddr*>(&su), sizeof(su)), 0) << strerror(errno);

    // send messages using the correct framing helper
    send_socket_message(cfd, std::string("subName"));
    send_socket_message(cfd, std::string("cmd1"));
    send_socket_message(cfd, std::string("cmd2"));

    // give the worker thread some time to dequeue & process
    for (int i = 0; i < 50 && CountingSession::ticks.load() == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // verify that every branch in handle_command() ran
    EXPECT_EQ(CountingSession::constructed.load(), 1);
    EXPECT_EQ(CountingSession::commands.load(), 2);
    EXPECT_GE(CountingSession::ticks.load(), 1);

    // clean up
    EXPECT_EQ(::close(cfd), 0);
}

TEST(UnixDomainServerHandleCommand, IdleClientTriggersDeleteBranch)
{
    std::string path;
    UnixDomainSockAddr addr = make_temp_addr_abstract_false(path);

    /* start server with an **empty factory**  */
    {
        UnixDomainServer server(addr, /*factory*/ {});

        std::this_thread::sleep_for(std::chrono::milliseconds(40));  // bind+listen

        /* connect client but **do not send** anything */
        int cli = ::socket(AF_UNIX, SOCK_STREAM, 0);
        ASSERT_GE(cli, 0) << strerror(errno);

        sockaddr_un su{};
        su.sun_family = AF_UNIX;
        std::strncpy(su.sun_path, path.c_str(), sizeof(su.sun_path) - 1);

        ASSERT_EQ(::connect(cli, reinterpret_cast<sockaddr*>(&su), sizeof(su)), 0) << strerror(errno);

        /* wait long enough for the 500 ms idle‑timeout to expire */
        std::this_thread::sleep_for(std::chrono::milliseconds(650));

        /* peer should have been closed by the server branch -> recv() returns 0 (EOF) */
        char dummy;
        ssize_t n = ::recv(cli, &dummy, 1, 0);
        EXPECT_EQ(n, 0) << "socket still open - idle branch not executed?";

        ::close(cli);  // tidy‑up client fd
    }  // server destructor runs here; should not abort

    SUCCEED();  // reaching here proves the branch ran
}

class CommandErrorInjectingSession : public UnixDomainServer::ISession
{
  public:
    explicit CommandErrorInjectingSession(UnixDomainServer::SessionHandle) {}
    bool tick() override
    {
        return false;
    }
    void on_command(const std::string&) override
    {
        throw std::runtime_error("boom!");
    }
};

TEST(UnixDomainServerExceptions, CatchStdExceptionInServerRoutine)
{
    /* prepare a non‑abstract pathname and start a real server*/
    std::string path;
    UnixDomainSockAddr addr = make_temp_addr_abstract_false(path);
    ::unlink(path.c_str());  // clean up any stale
    /* factory builds a session whose **on_command() causes error**. */
    UnixDomainServer::SessionFactory factory = [](const std::string&, UnixDomainServer::SessionHandle h) {
        return std::make_unique<CommandErrorInjectingSession>(h);
    };

    /* start the server (on its background thread) */
    UnixDomainServer server(addr, factory);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // let bind()

    /* create a client, subscribe (no throw), then send a 2nd string which triggers
     * CommandErrorInjectingSession::on_command() */
    int cfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(cfd, 0) << strerror(errno);

    sockaddr_un sun{};
    sun.sun_family = AF_UNIX;
    std::strncpy(sun.sun_path, path.c_str(), sizeof(sun.sun_path) - 1);
    ASSERT_EQ(0, ::connect(cfd, reinterpret_cast<sockaddr*>(&sun), sizeof(sun))) << strerror(errno);

    /* first message -> session is created */
    const char* subscribe = "subName";
    ASSERT_EQ(::send(cfd, subscribe, strlen(subscribe), 0), static_cast<ssize_t>(strlen(subscribe)));

    /* second message -> on_command() causes error */
    const char* badMsg = "this_will_cause_error";
    ASSERT_EQ(::send(cfd, badMsg, strlen(badMsg), 0), static_cast<ssize_t>(strlen(badMsg)));

    /* wait long enough for the poll‑loop to run exactly once */
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    /* Verify the socket was closed on the server side inside the
           catch‑handler: further send() must now fail with EPIPE. */
    const char* more = "ping";
    ssize_t sret = ::send(cfd, more, strlen(more), MSG_NOSIGNAL);
    EXPECT_EQ(sret, -1);
    EXPECT_TRUE(errno == EPIPE || errno == ECONNRESET);

    ::close(cfd);
}

class CloseAwareSession : public UnixDomainServer::ISession
{
  public:
    explicit CloseAwareSession(UnixDomainServer::SessionHandle /*h*/) {}

    bool tick() override
    {
        return false;
    }  // no re‑queue
    void on_closed_by_peer() override
    {
        ++peer_closed;
    }  // mark call

    static std::atomic<int> peer_closed;
};

std::atomic<int> CloseAwareSession::peer_closed{0};

TEST(UnixDomainServer, NotifiesClosedByPeer)
{
    CloseAwareSession::peer_closed = 0;

    // start server on fresh pathname
    std::string path;
    UnixDomainSockAddr addr = make_temp_addr_abstract_false(path);
    ::unlink(path.c_str());

    UnixDomainServer::SessionFactory factory = [](const std::string&, UnixDomainServer::SessionHandle h) {
        return std::make_unique<CloseAwareSession>(std::move(h));
    };

    UnixDomainServer server(addr, factory);

    std::this_thread::sleep_for(std::chrono::milliseconds(60));  // bind & listen

    // create client, subscribe, then close() to simulate peer hang‑up
    int cfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(cfd, 0) << strerror(errno);

    sockaddr_un su{};
    su.sun_family = AF_UNIX;
    std::strncpy(su.sun_path, path.c_str(), sizeof(su.sun_path) - 1);

    ASSERT_EQ(::connect(cfd, reinterpret_cast<sockaddr*>(&su), sizeof(su)), 0) << strerror(errno);

    send_socket_message(cfd, std::string("subscriber"));

    // grace interval for server to accept and build the session
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    ASSERT_EQ(::close(cfd), 0);

    // wait until the worker thread runs and calls notify_closed_by_peer()
    for (int i = 0; i < 50 && CloseAwareSession::peer_closed.load() == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // verify the callback was invoked exactly once
    EXPECT_EQ(CloseAwareSession::peer_closed.load(), 1);
}

class RequeueSession : public UnixDomainServer::ISession
{
  public:
    explicit RequeueSession(UnixDomainServer::SessionHandle /*h*/) {}

    bool tick() override
    {
        ++ticks;
        return ticks.load() == 1;  // true once, false afterwards
    }
    static std::atomic<int> ticks;
};

std::atomic<int> RequeueSession::ticks{0};

static UnixDomainServer::SessionFactory make_factory()
{
    return [](const std::string&, UnixDomainServer::SessionHandle h) {
        return std::make_unique<RequeueSession>(std::move(h));
    };
}

TEST(UnixDomainServer, WorkerRequeuesOnTrueTick)
{
    RequeueSession::ticks = 0;

    std::string path;
    UnixDomainSockAddr addr = make_temp_addr_abstract_false(path);
    ::unlink(path.c_str());

    UnixDomainServer server(addr, make_factory());

    std::this_thread::sleep_for(std::chrono::milliseconds(60));

    int cfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(cfd, 0) << strerror(errno);

    sockaddr_un su{};
    su.sun_family = AF_UNIX;
    std::strncpy(su.sun_path, path.c_str(), sizeof(su.sun_path) - 1);

    ASSERT_EQ(::connect(cfd, reinterpret_cast<sockaddr*>(&su), sizeof(su)), 0) << strerror(errno);

    // send a name so server builds the session
    send_socket_message(cfd, std::string("subscriber"));

    // wait until the worker thread has processed the session at least twice
    // (first tick returns true -> re‑queue, second tick returns false)
    for (int i = 0; i < 30 && RequeueSession::ticks.load() < 2; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_GE(RequeueSession::ticks.load(), 2);

    ::close(cfd);  // tidy up client side
}

class ErrorInjectingSession : public UnixDomainServer::ISession
{
  public:
    explicit ErrorInjectingSession(UnixDomainServer::SessionHandle /*h*/) {}
    bool tick() override
    {
        throw std::runtime_error("boom from tick()");
    }
    void on_command(const std::string&) override {}
};

// simple factory that builds a ErrorInjectingSession
static UnixDomainServer::SessionFactory kErrorInjectingFactory = [](const std::string&,
                                                                    UnixDomainServer::SessionHandle h) {
    return std::make_unique<ErrorInjectingSession>(h);
};

TEST(UnixDomainServer, ServerCatchesSessionFailure)
{
    std::string path;
    auto addr = make_temp_addr_abstract_false(path);
    UnixDomainServer server(addr, kErrorInjectingFactory);

    /* give server time to bind & listen */
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    int cfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_GE(cfd, 0) << strerror(errno);

    sockaddr_un su{};
    su.sun_family = AF_UNIX;
    std::strncpy(su.sun_path, path.c_str(), sizeof(su.sun_path) - 1);

    ASSERT_EQ(::connect(cfd, reinterpret_cast<sockaddr*>(&su), sizeof(su)), 0) << strerror(errno);

    const char* sub = "subName";
    ssize_t n = ::send(cfd, sub, std::strlen(sub), 0);
    ASSERT_EQ(n, static_cast<ssize_t>(std::strlen(sub))) << strerror(errno);

    /* give the server thread enough time to dequeue & run tick() (⇒ error) */
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    /* clean up the client socket */
    ::close(cfd);
}

extern "C" {

// test‑controlled return‑code for shm_create_handle
static int g_shm_create_rc = 0;  // default: succeed

int shm_create_handle(int /*fd*/, int /*pid*/, int /*oflag*/, void* /*handle*/, int /*reserved*/)
{
    // just return whatever the test configured
    return g_shm_create_rc;
}
}  // extern "C"

using namespace testing;

TEST(UnixDomainServerSocketFixture, ServerFailedToCreateSocket)
{
    std::unique_ptr<score::os::SocketMock> sock_mock;
    sock_mock = std::make_unique<score::os::SocketMock>();
    ::testing::Mock::AllowLeak(sock_mock.get());
    score::os::Socket::set_testing_instance(*sock_mock);
    EXPECT_CALL(*sock_mock, socket(_, _, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    EXPECT_CALL(*sock_mock, bind(_, _, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(*sock_mock, listen(_, _)).WillRepeatedly(Return(score::cpp::blank{}));
    UnixDomainSockAddr addr("socket", true);
    EXPECT_DEATH(UnixDomainServer server(addr, UnixDomainServer::SessionFactory()), "");
}

TEST(UnixDomainServerSocketFixture, ServerFailedToCreateBind)
{
    std::unique_ptr<score::os::SocketMock> sock_mock;
    sock_mock = std::make_unique<score::os::SocketMock>();
    score::os::Socket::set_testing_instance(*sock_mock);
    ::testing::Mock::AllowLeak(sock_mock.get());
    EXPECT_CALL(*sock_mock, socket(_, _, _)).WillRepeatedly(Return(20));
    EXPECT_CALL(*sock_mock, bind(_, _, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    UnixDomainSockAddr addr("socket", true);
    EXPECT_DEATH(UnixDomainServer server(addr, UnixDomainServer::SessionFactory()), "");
}

TEST(UnixDomainServerSocketFixture, ServerFailedToListen)
{
    std::unique_ptr<score::os::SocketMock> sock_mock;
    sock_mock = std::make_unique<score::os::SocketMock>();
    score::os::Socket::set_testing_instance(*sock_mock);
    ::testing::Mock::AllowLeak(sock_mock.get());
    EXPECT_CALL(*sock_mock, socket(_, _, _)).WillRepeatedly(Return(20));
    EXPECT_CALL(*sock_mock, bind(_, _, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(*sock_mock, listen(_, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    UnixDomainSockAddr addr("socket", true);
    EXPECT_DEATH(UnixDomainServer server(addr, UnixDomainServer::SessionFactory()), "");
}

TEST(UnixDomainServerSocketFixture, FailedToPoll)
{
    std::string path_ = "/tmp/uds_accept_test_" + std::to_string(getpid());
    ::unlink(path_.c_str());

    score::os::SysPollMock sysPollMock;
    score::os::SocketMock sock_mock;

    ::testing::Mock::AllowLeak(&sock_mock);
    ::testing::Mock::AllowLeak(&sysPollMock);

    score::os::Socket::set_testing_instance(sock_mock);
    score::os::SysPoll::set_testing_instance(sysPollMock);

    EXPECT_CALL(sock_mock, socket(_, _, _)).WillRepeatedly(Return(20));
    EXPECT_CALL(sock_mock, bind(_, _, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(sock_mock, listen(_, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(sysPollMock, poll(_, _, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    EXPECT_CALL(sock_mock, accept(_, _, _)).WillRepeatedly(Return(1));

    UnixDomainSockAddr addr_(path_, /* isAbstract = */ false);

    ASSERT_DEATH(
        {
            // This block is expected to call std::exit()
            UnixDomainServer* server_ = new UnixDomainServer(addr_, kFactory);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            delete server_;
        },
        ".*poll.*");
}

TEST(UnixDomainServerSocketFixture, ServerFailedToAcceptClientConnection)
{
    score::os::SysPollMock sysPollMock;
    score::os::SocketMock sock_mock;

    ::testing::Mock::AllowLeak(&sock_mock);
    ::testing::Mock::AllowLeak(&sysPollMock);

    score::os::Socket::set_testing_instance(sock_mock);
    score::os::SysPoll::set_testing_instance(sysPollMock);

    EXPECT_CALL(sock_mock, socket(_, _, _)).WillRepeatedly(Return(20));
    EXPECT_CALL(sock_mock, bind(_, _, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(sock_mock, listen(_, _)).WillRepeatedly(Return(score::cpp::blank{}));
    EXPECT_CALL(sysPollMock, poll(_, _, _)).WillRepeatedly([](struct pollfd* in_pollfd, nfds_t, int) {
        in_pollfd[0].fd = 20;  // return fd as 1 to call ARPfilter functions
        in_pollfd[0].revents = POLLIN;
        return 1;  // number of events for polling
    });

    std::string path;
    UnixDomainSockAddr addr = make_temp_addr_abstract_false(path);

    std::this_thread::sleep_for(std::chrono::milliseconds(40));  // bind+listen
    EXPECT_CALL(sock_mock, accept(_, _, _))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));
    // open one client
    auto client_socket_ret = score::os::Socket::instance().socket(score::os::Socket::Domain::kUnix, SOCK_STREAM, 0);
    std::int32_t client_fd = client_socket_ret.has_value() ? client_socket_ret.value() : -1;

    ASSERT_GE(client_fd, 0) << strerror(errno);

    ASSERT_DEATH(
        {
            // This block is expected to call std::exit()
            UnixDomainServer* server_ = new UnixDomainServer(addr, kFactory);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            delete server_;
        },
        ".*accept.*");
}

class PthreadMockFixture : public ::testing::Test
{
  protected:
    // The mock object (lives as long as the fixture)
    score::os::MockPthread pthreadMock_;

    void SetUp() override
    {
        // Tell the singleton to use *our* mock instead of the real impl
        score::os::Pthread::set_testing_instance(pthreadMock_);

        // Default behaviour: pretend everything succeeds
        ON_CALL(pthreadMock_, setname_np(::testing::_, ::testing::_)).WillByDefault([](pthread_t, const char*) {
            return score::cpp::blank{};  // success
        });
    }

    void TearDown() override
    {
        // Remove the testing instance so it does not leak into other tests
        score::os::Pthread::set_testing_instance(score::os::Pthread::instance());
    }
};

TEST_F(PthreadMockFixture, ServerIgnoresSetnameError)
{
    score::os::SysPollMock sysPollMock;
    score::os::SocketMock sock_mock;

    score::os::Socket::set_testing_instance(sock_mock);
    score::os::SysPoll::set_testing_instance(sysPollMock);
    ::testing::Mock::AllowLeak(&sock_mock);
    ::testing::Mock::AllowLeak(&sysPollMock);

    // Make the call appear to fail (Error::createFromErrno() is the usual way)
    EXPECT_CALL(pthreadMock_, setname_np(::testing::_, ::testing::_))
        .Times(1)
        .WillRepeatedly(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
    EXPECT_CALL(sock_mock, socket(_, _, _)).WillRepeatedly(Return(30));
    EXPECT_CALL(sock_mock, socket(_, _, _)).WillRepeatedly(Return(30));
    EXPECT_CALL(sock_mock, bind(_, _, _)).Times(Exactly(1)).WillOnce(Return(score::cpp::blank{}));
    EXPECT_CALL(sock_mock, listen(_, _)).Times(Exactly(1)).WillOnce(Return(score::cpp::blank{}));
    EXPECT_CALL(sysPollMock, poll(_, _, _)).WillRepeatedly([](struct pollfd* in_pollfd, nfds_t, int) {
        in_pollfd[0].fd = 1;  // return fd as 1 to call ARPfilter functions
        in_pollfd[0].revents = POLLIN;
        return 1;  // number of events for polling
    });
    EXPECT_CALL(sock_mock, accept(_, _, _)).WillRepeatedly(Return(30));

    // happens and the process does *not* abort when the error is returned.
    UnixDomainSockAddr addr("uds_mock_socket", /*isAbstract=*/true);
    {
        UnixDomainServer server(addr, UnixDomainServer::SessionFactory{});
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // server dtor will run here; test passes if no abort & expectation met
    }
}

TEST(UnixDomainCommon, SetupSignalsFailurePath)
{
    // Given we have our mock gives us the error path on all calls
    std::unique_ptr<StrictMock<score::os::SignalMock>> signal_mock = std::make_unique<StrictMock<score::os::SignalMock>>();
    auto error = score::os::Error::createUnspecifiedError();

    testing::InSequence seq;
    EXPECT_CALL(*signal_mock, SigEmptySet(_)).WillOnce(Return(score::cpp::make_unexpected(error)));
    EXPECT_CALL(*signal_mock, SigAddSet(_, _)).WillOnce(Return(score::cpp::make_unexpected(error)));
    EXPECT_CALL(*signal_mock, PthreadSigMask(_, _)).WillOnce(Return(score::cpp::make_unexpected(error)));
    EXPECT_CALL(*signal_mock, SigAction(_, _, _)).WillOnce(Return(score::cpp::make_unexpected(error)));

    std::unique_ptr<score::os::Signal> s(std::move(signal_mock));

    // When we capture the output
    std::cerr.flush();
    testing::internal::CaptureStderr();

    SetupSignals(s);

    std::cerr.flush();
    std::string output = testing::internal::GetCapturedStderr();
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
    // Test orphaned FD cleanup in process_idle_connections
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

    ASSERT_EQ(state.connection_pollfd_list.size(), 2u);
    ASSERT_EQ(state.connection_fd_map.size(), 0u);

    UnixDomainServer::UnixDomainServerTest::process_idle_connections(state);

    // Orphaned FD removed, only server FD remains
    ASSERT_EQ(state.connection_pollfd_list.size(), 1u);
    ASSERT_FALSE(state.connection_pollfd_list.empty());
    if (!state.connection_pollfd_list.empty())
    {
        EXPECT_EQ(state.connection_pollfd_list[0].fd, 1);
    }
    EXPECT_EQ(state.connection_fd_map.size(), 0u);
}

TEST(UnixDomainServerExtractedMethods, ProcessIdleConnectionsSkipsActiveConnections)
{
    // Test that active connections (with POLLIN) are skipped by process_idle_connections
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

    ASSERT_EQ(state.connection_pollfd_list.size(), 2u);

    UnixDomainServer::UnixDomainServerTest::process_idle_connections(state);

    // Size unchanged: process_idle_connections() should skip active connections (POLLIN set)
    // and only process idle ones, so both FDs remain in the list
    EXPECT_EQ(state.connection_pollfd_list.size(), 2u);
}

TEST(UnixDomainServerExtractedMethods, ProcessActiveConnectionsRemovesOrphanedFd)
{
    // Test orphaned FD cleanup in process_active_connections
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

    ASSERT_EQ(state.connection_pollfd_list.size(), 2u);
    ASSERT_EQ(state.connection_fd_map.size(), 0u);

    UnixDomainServer::UnixDomainServerTest::process_active_connections(state);

    ASSERT_EQ(state.connection_pollfd_list.size(), 1u);
    EXPECT_EQ(state.connection_pollfd_list[0].fd, 1);
    EXPECT_EQ(state.connection_fd_map.size(), 0u);
}

}  // namespace
}  // namespace internal
}  // namespace platform
}  // namespace score
