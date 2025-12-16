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

#include "unix_domain/unix_domain_server.h"
#include "score/os/pthread.h"

#include "score/os/socket.h"
#include "score/os/sys_poll.h"
#include "score/os/unistd.h"
#include "score/os/utils/signal_impl.h"
#include <sys/mman.h>
#include <unistd.h>

#include <fcntl.h>
#include <iostream>
#include <thread>
#include <vector>

namespace score
{
namespace platform
{
namespace internal
{

UnixDomainServer::SessionWrapper::~SessionWrapper()
{
    if (session_fd_ != -1)
    {
        // Suppressed here as it is safely used, and it is among safety headers.
        // NOLINTNEXTLINE(score-banned-function) see comment above
        score::os::Unistd::instance().close(session_fd_);
    }
}

bool UnixDomainServer::SessionWrapper::handle_command(const std::string& in_string,
                                                      score::cpp::optional<std::int32_t> peer_pid)
{
    if (nullptr == session_)
    {
        if (!in_string.empty() && server_->factory_)
        {
            // subscriber session
            if (false == peer_pid.has_value())
            {
                // Execution reaches this diagnostic only in an abort‑path (missing peer‑PID); exercising it in a
                // unit‑test would terminate the process, so the line is excluded from coverage.
                std::cerr << "UnixDomainServer: Peer PID unavailable" << std::endl;  // LCOV_EXCL_LINE
            }
            session_ = server_->factory_(in_string, SessionHandle{session_fd_});
        }
        else
        {
            return timestamp_t::clock::now() < timeout_;
        }
    }
    else
    {
        if (!in_string.empty())
        {
            session_->on_command(in_string);
        }
    }
    enqueue_tick();
    return true;
}

bool UnixDomainServer::SessionWrapper::try_enqueue_for_delete(bool by_peer)
{
    if (nullptr != session_)
    {
        to_delete_ = true;
        closed_by_peer_ = by_peer;
        if (!running_ && !enqueued_)
        {
            server_->enqueue_tick_direct(session_fd_);
            enqueued_ = true;
        }
        return true;
    }
    else
    {
        // no session to finish, can be deleted outright
        return false;
    }
}

bool UnixDomainServer::SessionWrapper::tick()
{
    bool requeue = session_->tick();
    return requeue;
}

void UnixDomainServer::SessionWrapper::notify_closed_by_peer()
{
    session_->on_closed_by_peer();
}

void UnixDomainServer::SessionWrapper::set_running()
{
    enqueued_ = false;
    running_ = true;
}

bool UnixDomainServer::SessionWrapper::reset_running(bool requeue)
{
    running_ = false;
    if (requeue)
    {
        enqueued_ = true;
    }
    return enqueued_;
}

void UnixDomainServer::SessionWrapper::enqueue_tick()
{
    if (!enqueued_ && !to_delete_)
    {
        if (!running_)
        {
            server_->enqueue_tick_direct(session_fd_);
        }
        enqueued_ = true;
    }
}

void UnixDomainServer::server_routine(UnixDomainSockAddr addr)
{
    SetupSignals(signal_);

    const std::int32_t server_fd = setup_server_socket(addr);

    // Create connection state struct with server file descriptor
    // The first element in the pollfd list is special - it is the server file descriptor.
    // All other file descriptors belong to clients accepted by the server.
    ConnectionState state{};
    state.connection_pollfd_list.push_back({server_fd, POLLIN, 0});

    using timestamp_t = std::chrono::steady_clock::time_point;
    timestamp_t t1 = timestamp_t::clock::now() + std::chrono::milliseconds(100);
    while (false == server_exit_.load())
    {
        std::int32_t timeout = static_cast<std::int32_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - timestamp_t::clock::now()).count());

        if (timeout <= 0)
        {
            timeout = 0;
            t1 = timestamp_t::clock::now() + std::chrono::milliseconds(100);
        }

        process_server_iteration(state, server_fd, timeout);
    }  //  while (false == server_exit_.load())

    // Cleanup all connections on shutdown
    cleanup_all_connections(state);

    // Suppressed here as it is safely used, and it is among safety headers.
    // NOLINTNEXTLINE(score-banned-function) see comment above
    score::cpp::ignore = score::os::Unistd::instance().close(server_fd);
}

std::int32_t UnixDomainServer::setup_server_socket(UnixDomainSockAddr& addr)
{
    if (!addr.is_abstract())
    {
        const auto unlink_ret = score::os::Unistd::instance().unlink(static_cast<const char*>(addr.addr_.sun_path));
        if (!unlink_ret.has_value())
        {
            std::perror("unlink");
            std::fprintf(stderr, "address: %s\n", static_cast<const char*>(addr.addr_.sun_path));
        }
    }
    const auto socket_ret = score::os::Socket::instance().socket(score::os::Socket::Domain::kUnix, SOCK_STREAM, 0);
    if (!socket_ret.has_value())
    {
        std::perror("Socket");
        // NOLINTNEXTLINE(score-banned-function): Suppressed here because of error handling
        std::exit(EXIT_FAILURE);
    }
    const std::int32_t server_fd = socket_ret.value();
    const auto bind_ret = score::os::Socket::instance().bind(
        server_fd, static_cast<const sockaddr*>(static_cast<const void*>(&addr)), sizeof(sockaddr_un));
    if (!bind_ret.has_value())
    {
        std::perror("bind");
        std::cerr << "address: " << addr.get_address_string() << std::endl;
        // NOLINTNEXTLINE(score-banned-function): Suppressed here because of error handling
        std::exit(EXIT_FAILURE);
    }
    // Suppressed here as it is safely used, and it is among safety headers.
    // NOLINTNEXTLINE(score-banned-function) see comment above
    const auto listen_ret = score::os::Socket::instance().listen(server_fd, 20);
    if (!listen_ret.has_value())
    {
        std::perror("listen");
        // NOLINTNEXTLINE(score-banned-function): Suppressed here because of error handling
        std::exit(EXIT_FAILURE);
    }
    return server_fd;
}

void UnixDomainServer::process_server_iteration(ConnectionState& state,
                                                const std::int32_t server_fd,
                                                const std::int32_t timeout)
{
    const auto size = state.connection_pollfd_list.size();
    //  Suppressed here as it is safely used, and it is among safety headers.
    //  NOLINTBEGIN(score-banned-function) see comment above
    score::cpp::expected<std::int32_t, score::os::Error> poll_ret;
#ifdef __QNX__
// NOLINTBEGIN(score-banned-preprocessor-directives) : required due to compiler warning for qnx
/*
Deviation from Rule A16-7-1:
- The #pragma directive shall not be used
Justification:
- required due to compiler warning for qnx
*/
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic push
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic ignored "-Wuseless-cast"
    poll_ret =
        score::os::SysPoll::instance().poll(state.connection_pollfd_list.data(), static_cast<nfds_t>(size), timeout);
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic pop
// NOLINTEND(score-banned-preprocessor-directives)
#else
    poll_ret = score::os::SysPoll::instance().poll(state.connection_pollfd_list.data(), size, timeout);
#endif

    // NOLINTEND(score-banned-function)
    if (!poll_ret.has_value())
    {
        std::perror("poll");
        // NOLINTNEXTLINE(score-banned-function): Suppressed here because of error handling
        std::exit(EXIT_FAILURE);
    }
    if (0 != (state.connection_pollfd_list[0].revents & POLLIN))  //  poll server activities
    {
        // Suppressed here as it is safely used, and it is among safety headers.
        // NOLINTNEXTLINE(score-banned-function) see comment above
        const auto ret_data_fd = score::os::Socket::instance().accept(server_fd, nullptr, nullptr);
        if (!ret_data_fd.has_value())
        {
            std::perror("accept");
            // NOLINTNEXTLINE(score-banned-function): Suppressed here because of error handling
            std::exit(EXIT_FAILURE);
        }
        state.connection_pollfd_list.push_back({ret_data_fd.value(), POLLIN, 0});
        state.connection_fd_map.insert(std::make_pair(ret_data_fd.value(), SessionWrapper{this, ret_data_fd.value()}));
    }

    // Process connections with incoming data
    process_active_connections(state);

    process_queue(state.connection_fd_map);

    if (timeout > 0)
    {
        return;  // Skip idle connection processing when timeout hasn't expired
    }

    // Process idle connections
    process_idle_connections(state);

    process_queue(state.connection_fd_map);
}

void UnixDomainServer::process_active_connections(ConnectionState& state)
{
    auto server_fd_advance_iterator = state.connection_pollfd_list.begin();
    if (server_fd_advance_iterator != state.connection_pollfd_list.end())
    {
        ++server_fd_advance_iterator;
        // Deviation from Rule A6-5-2:
        // - Cannot use numeric loop counter because we erase elements inside the loop
        // Justification:
        // - Iterator must be manually incremented to safely handle erase operations
        // coverity[autosar_cpp14_a6_5_2_violation]
        for (auto it = server_fd_advance_iterator; it != state.connection_pollfd_list.end();
             /*iterate or reassign when erasing inside the loop*/)
        {
            if (0 != (it->revents & POLLIN))
            {
                const std::int32_t session_fd = it->fd;
                auto session_iterator = state.connection_fd_map.find(session_fd);
                if (session_iterator != state.connection_fd_map.end())
                {
                    auto& [fd_map, session] = *session_iterator;
                    score::cpp::optional<std::int32_t> in_pid = score::cpp::nullopt;
                    score::cpp::optional<std::int32_t> file_handle = score::cpp::nullopt;
                    //  File descriptor is no longer sent from client to server and server uses universal API and
                    //  thus FD is discarded
                    const auto response = recv_socket_message(session_fd, file_handle, in_pid);
                    if (!response.has_value() || !session.handle_command(response.value(), in_pid))
                    {
                        const bool delayed = session.try_enqueue_for_delete(true);
                        it = state.connection_pollfd_list.erase(it);

                        if (!delayed)
                        {
                            state.connection_fd_map.erase(session_fd);
                        }
                    }
                    else
                    {
                        // manually increment next element:
                        it++;
                    }
                }
                else  //  element is missing in the other map:
                {
                    //  Assign iterator new value after erase:
                    it = state.connection_pollfd_list.erase(it);
                }
            }
            else
            {
                it++;
            }
        }
    }
}

void UnixDomainServer::process_idle_connections(ConnectionState& state)
{
    //  Process elements that are idle i.e. pollfd did not report any events for those elements:
    auto server_fd_advance_iterator = state.connection_pollfd_list.begin();
    if (server_fd_advance_iterator != state.connection_pollfd_list.end())
    {
        ++server_fd_advance_iterator;
        // Deviation from Rule A6-5-2:
        // - Cannot use numeric loop counter because we erase elements inside the loop
        // Justification:
        // - Iterator must be manually incremented to safely handle erase operations
        // coverity[autosar_cpp14_a6_5_2_violation]
        for (auto it = server_fd_advance_iterator; it != state.connection_pollfd_list.end();
             /*iterate or reassign when erasing inside the loop*/)
        {
            if (0 == (it->revents & POLLIN))
            {
                const std::int32_t session_fd = it->fd;
                auto session_it = state.connection_fd_map.find(session_fd);
                if (session_it != state.connection_fd_map.end())
                {
                    auto& [fd_map, session] = *session_it;
                    std::string in_string;
                    if (!session.handle_command(in_string))
                    {
                        const bool delayed = session.try_enqueue_for_delete();
                        it = state.connection_pollfd_list.erase(it);
                        if (!delayed)
                        {
                            state.connection_fd_map.erase(session_fd);
                        }
                    }
                    else
                    {
                        it++;
                    }
                }
                else
                {
                    it = state.connection_pollfd_list.erase(it);
                }
            }
            else
            {
                ++it;
            }
        }
    }
}

void UnixDomainServer::cleanup_all_connections(ConnectionState& state)
{
    //  Go over all elements
    auto server_fd_advance_iterator = state.connection_pollfd_list.begin();
    if (server_fd_advance_iterator != state.connection_pollfd_list.end())
    {
        ++server_fd_advance_iterator;
        // Deviation from Rule A6-5-2:
        // - Cannot use numeric loop counter because we erase elements inside the loop
        // Justification:
        // - Iterator must be manually incremented to safely handle erase operations
        // coverity[autosar_cpp14_a6_5_2_violation]
        for (auto it = server_fd_advance_iterator; it != state.connection_pollfd_list.end();
             /*iterate or reassign when erasing inside the loop*/)
        {
            const std::int32_t session_fd = it->fd;
            auto session_it = state.connection_fd_map.find(session_fd);
            if (session_it != state.connection_fd_map.end())
            {
                auto& [fd_map, session] = *session_it;
                const bool delayed = session.try_enqueue_for_delete();
                if (!delayed)
                {
                    state.connection_fd_map.erase(session_fd);
                }
            }
            it = state.connection_pollfd_list.erase(it);
        }
    }
    state.connection_fd_map.clear();
}

bool UnixDomainServer::process_queue(std::unordered_map<int, SessionWrapper>& connection_fd_map)
{
    // Deviation from Rule A6-5-2:
    // - Numeric loop counter cannot be used because work_queue_ is popped inside the loop
    // Justification:
    // - Iteration over queue safely pops elements until empty
    // coverity[autosar_cpp14_a6_5_2_violation]
    for (/*empty*/; !work_queue_.empty(); work_queue_.pop())
    {
        const std::int32_t fd = work_queue_.front();
        auto wrapper_iterator = connection_fd_map.find(fd);
        if (wrapper_iterator != connection_fd_map.end())
        {
            auto& [fd_map, wrapper] = *wrapper_iterator;
            wrapper.set_running();
            const bool closed_by_peer = wrapper.get_reset_closed_by_peer();
            if (closed_by_peer)
            {
                wrapper.notify_closed_by_peer();
            }
            const bool requeue = wrapper.tick();

            if (wrapper.reset_running(requeue))
            {
                enqueue_tick_direct(fd);
            }
            else if (wrapper.is_marked_for_delete())
            {
                connection_fd_map.erase(fd);
            }
        }
        else
        {
            /* TODO: Element not found */
        }
    }
    return false;
}

void UnixDomainServer::enqueue_tick_direct(std::int32_t fd)
{
    work_queue_.push(fd);
}

void UnixDomainServer::pass_message(std::int32_t fd, const std::string& message)
{
    send_socket_message(fd, message);
}

void UnixDomainServer::update_thread_name_server_routine() noexcept
{
    auto ret = score::os::Pthread::instance().setname_np(server_thread_.native_handle(), "server_routine");
    if (!ret.has_value())
    {
        auto errstr = ret.error().ToString();
        std::cerr << "pthread_setname_np: " << errstr << std::endl;
    }
}

}  // namespace internal
}  // namespace platform
}  // namespace score
