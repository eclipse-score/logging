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

#ifndef UNIX_DOMAIN_SERVER_H_
#define UNIX_DOMAIN_SERVER_H_

#include "i_session.h"
#include "unix_domain/unix_domain_common.h"

#include <score/jthread.hpp>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <queue>
#include <thread>
#include <unordered_map>

#include "poll.h"

#include "score/os/utils/signal.h"
#include "score/os/utils/signal_impl.h"

namespace score
{
namespace platform
{
namespace internal
{

class UnixDomainServer
{
  public:
    class SessionHandle
    {
      public:
        explicit SessionHandle(int fd) : socket_descriptor_(fd) {}

        void pass_message(const std::string& message) const
        {
            UnixDomainServer::pass_message(socket_descriptor_, message);
        }

      private:
        int socket_descriptor_;
    };

    using ISession = score::logging::ISession;

    using SessionFactory = std::function<std::unique_ptr<ISession>(const std::string&, SessionHandle)>;

    class SessionWrapper
    {
      public:
        SessionWrapper(UnixDomainServer* server, int fd)
            : server_(server),
              session_fd_(fd),
              timeout_(timestamp_t::clock::now() + std::chrono::milliseconds(500)),
              enqueued_(false),
              running_(false),
              to_delete_(false),
              closed_by_peer_(false),
              session_{nullptr}
        {
        }
        ~SessionWrapper();
        SessionWrapper(const SessionWrapper&) = delete;
        SessionWrapper(SessionWrapper&& from) noexcept
            : server_{from.server_},
              session_fd_{from.session_fd_},
              timeout_{from.timeout_},
              enqueued_(false),
              running_(false),
              to_delete_(false),
              closed_by_peer_(false),
              session_{std::move(from.session_)}
        {
            from.session_fd_ = -1;
        }

        //  returns `false` when the session shall finish
        bool handle_command(const std::string& in_string, score::cpp::optional<std::int32_t> peer_pid = score::cpp::nullopt);
        // returns information about `session_` existance. If the session does not exists (false), any associated
        // context can be deleted outright, otherwise(true) delay erasing.
        bool try_enqueue_for_delete(bool by_peer = false);
        bool is_marked_for_delete()
        {
            return to_delete_;
        }

        bool get_reset_closed_by_peer()
        {
            bool by_peer = closed_by_peer_;
            closed_by_peer_ = false;
            return by_peer;
        }

        bool tick();
        void notify_closed_by_peer();

        void set_running();
        bool reset_running(bool requeue);

        class SessionWrapperTest;

      private:
        using timestamp_t = std::chrono::steady_clock::time_point;

        UnixDomainServer* server_;
        int session_fd_;
        timestamp_t timeout_;

        bool enqueued_;
        bool running_;
        bool to_delete_;
        bool closed_by_peer_;

        std::unique_ptr<ISession> session_;

        void enqueue_tick();
    };

    explicit UnixDomainServer(UnixDomainSockAddr addr,
                              SessionFactory factory = SessionFactory(),
                              std::unique_ptr<score::os::Signal> signal = std::make_unique<score::os::SignalImpl>())
        : server_exit_{false}, server_thread_{}, work_queue_{}, factory_(factory), signal_(std::move(signal))
    {
        server_thread_ = score::cpp::jthread([this, addr]() {
            server_routine(addr);
        });
        update_thread_name_server_routine();
    }

    virtual ~UnixDomainServer()
    {
        server_exit_ = true;
        server_thread_.join();
    }

    class UnixDomainServerTest;

  private:
    /*
    Deviation from Rule A11-3-1:
    - Friend declarations shall not be used
    Justification:
    - required to access the private member
    */
    // coverity[autosar_cpp14_a11_3_1_violation]
    friend class SessionWrapper;

    // Struct to hold connection state for easier testing and parameter passing
    struct ConnectionState
    {
        std::unordered_map<int, SessionWrapper> connection_fd_map;
        std::vector<pollfd> connection_pollfd_list;
    };

    static void pass_message(std::int32_t fd, const std::string& message);

    void server_routine(UnixDomainSockAddr addr);

    virtual void enqueue_tick_direct(std::int32_t fd);

    //  returns true if the queue is still not empty:
    bool process_queue(std::unordered_map<int, SessionWrapper>& connection_fd_map);

    // Methods to process connections
    static void process_active_connections(ConnectionState& state);
    static void process_idle_connections(ConnectionState& state);
    void cleanup_all_connections(ConnectionState& state);
    std::int32_t setup_server_socket(UnixDomainSockAddr& addr);
    void process_server_iteration(ConnectionState& state, const std::int32_t server_fd, const std::int32_t timeout);

    void update_thread_name_server_routine() noexcept;
    std::atomic_bool server_exit_;

    score::cpp::jthread server_thread_;
    std::queue<int> work_queue_;

    SessionFactory factory_;

    std::unique_ptr<score::os::Signal> signal_;
};

namespace dummy_namespace
{
// dummy type to avoid warnings Wsuggest-final-types and Wsuggest-final-methods
struct temp_marker final : UnixDomainServer::ISession
{
    bool tick() override
    {
        return false;
    };
    void on_command(const std::string&) override {}
    void on_closed_by_peer() override {}
};
}  // namespace dummy_namespace

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // UNIX_DOMAIN_SERVER_H_
