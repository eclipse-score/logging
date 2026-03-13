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

#include "daemon/message_passing_server.h"
#include "score/os/pthread.h"
#include "score/os/unistd.h"
#include "score/mw/log/detail/data_router/message_passing_config.h"

#include "score/memory.hpp"
#include <score/jthread.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

namespace score
{
namespace platform
{
namespace internal
{

using score::mw::log::detail::DatarouterMessageIdentifier;
using score::mw::log::detail::MessagePassingConfig;

void MessagePassingServer::SessionWrapper::EnqueueForDeleteWhileLocked(bool by_peer)
{
    to_delete = true;
    closed_by_peer = by_peer;
    // in order not to mess with the logic of the queue, we don't enqueue currently running tick. Instead, we mark it
    // to be deleted (or re-enqueued for post-mortem processing, if closed by peer) at the end of the tick processing
    if (!running && !enqueued)
    {
        server->EnqueueTickWhileLocked(pid);
        enqueued = true;
    }
}

bool MessagePassingServer::SessionWrapper::TickAtWorkerThread() const
{
    bool requeue = session->Tick();
    return requeue;
}

void MessagePassingServer::SessionWrapper::NotifyClosedByPeer() const
{
    session->OnClosedByPeer();
}

void MessagePassingServer::SessionWrapper::SetRunningWhileLocked()
{
    enqueued = false;
    running = true;
}

bool MessagePassingServer::SessionWrapper::ResetRunningWhileLocked(bool requeue)
{
    running = false;
    // check if we need to re-enqueue the tick after running again. It may happen because:
    // 1. not all the work in tick was done (returned early to avoid congestion);
    // 2. the tick was marked for delete as "closed by peer" when running, but we don't expedite its finishing.
    if (requeue || closed_by_peer)
    {
        enqueued = true;
    }
    return enqueued;
}

void MessagePassingServer::SessionWrapper::EnqueueTickWhileLocked()
{
    if (!enqueued && !to_delete)
    {
        if (!running)
        {
            server->EnqueueTickWhileLocked(pid);
        }
        enqueued = true;
    }
}
/*
    Deviation from Rule A3-1-1:
    - It shall be possible to include any header file
     in multiple translation units without violating the One Definition Rule.
    Justification:
    - This is false positive. Function is declared only once.
*/
// coverity[autosar_cpp14_a3_1_1_violation]
MessagePassingServer::MessagePassingServer(MessagePassingServer::SessionFactory factory,
                                           std::shared_ptr<score::message_passing::IServerFactory> server_factory,
                                           AcquireWatchdogConfig watchdog_config)
    : IMessagePassingServerSessionWrapper(),
      factory_{std::move(factory)},
      mutex_{},
      stop_source_{},
      connection_timeout_{},
      worker_cond_{},
      pid_session_map_{},
      work_queue_{},
      workers_exit_{false},
      server_cond_{},
      session_finishing_{false},
      server_factory_{server_factory},
      watchdog_config_{watchdog_config}
{
    worker_thread_ = score::cpp::jthread([this]() {
        RunWorkerThread();
    });

    auto ret_pthread = score::os::Pthread::instance().setname_np(worker_thread_.native_handle(), "mp_worker");
    if (!ret_pthread.has_value())
    {
        std::cerr << "setname_np: " << ret_pthread.error() << std::endl;
    }

    constexpr score::message_passing::ServiceProtocolConfig kServiceProtocolConfig{
        MessagePassingConfig::kDatarouterReceiverIdentifier,
        MessagePassingConfig::kMaxMessageSize,
        MessagePassingConfig::kMaxReplySize,
        MessagePassingConfig::kMaxNotifySize};

    constexpr score::message_passing::IServerFactory::ServerConfig kServerConfig{
        MessagePassingConfig::kMaxReceiverQueueSize,
        MessagePassingConfig::kPreAllocConnections,
        MessagePassingConfig::kMaxQueuedNotifies};
    receiver_ = server_factory_->Create(kServiceProtocolConfig, kServerConfig);

    auto connect_callback = [this_ptr =
                                 this](score::message_passing::IServerConnection& connection) noexcept -> std::uintptr_t {
        const pid_t client_pid = connection.GetClientIdentity().pid;
        {
            std::lock_guard<std::mutex> lock(this_ptr->mutex_);
            this_ptr->disconnected_pids_.erase(client_pid);
        }
        return static_cast<std::uintptr_t>(client_pid);
    };
    auto disconnect_callback = [this_ptr = this](score::message_passing::IServerConnection& connection) noexcept {
        const pid_t client_pid = connection.GetClientIdentity().pid;
        std::unique_lock<std::mutex> lock(this_ptr->mutex_);
        this_ptr->disconnected_pids_.insert(client_pid);

        const auto found = this_ptr->pid_session_map_.find(client_pid);
        if (found != this_ptr->pid_session_map_.end())
        {
            SessionWrapper& wrapper = found->second;
            wrapper.connection = nullptr;
            wrapper.to_force_finish = true;
            found->second.EnqueueForDeleteWhileLocked(true);
        }
    };
    auto received_send_message_callback = [this_ptr = this](
                                              score::message_passing::IServerConnection& connection,
                                              const score::cpp::span<const std::uint8_t> message) noexcept -> score::cpp::blank {
        this_ptr->MessageCallback(connection, message);
        return {};
    };
    auto received_send_message_with_reply_callback =
        [](score::message_passing::IServerConnection& /*connection*/,
           score::cpp::span<const std::uint8_t> /*message*/) noexcept -> score::cpp::blank {
        return {};
    };

    auto ret_listening = receiver_->StartListening(connect_callback,
                                                   disconnect_callback,
                                                   received_send_message_callback,
                                                   received_send_message_with_reply_callback);
    if (!ret_listening)
    {
        std::cerr << "StartListening: " << ret_listening.error() << std::endl;
    }
}

/*
Deviation from Rule A15-5-1:
- All user-provided class destructors, deallocation functions, move constructors,
- move assignment operators and swap functions shall not exit with an exception.
- A noexcept exception specification shall be added to these functions as appropriate.
Justification:
- Ensure that worker_thread_ is not running after destruction of MessagePassingServer
- checking worker_thread_.joinable() should be enough to avoid exception from join().
- in this case join() could throw exception only if something goes wrong on OS level.
- this should be fine, moreover it could happen only on system shutdown stage
- and does not affect normal runtime
*/
// coverity[autosar_cpp14_a15_5_1_violation] see above
MessagePassingServer::~MessagePassingServer() noexcept
{
    // first, unblock the possible client connection requests
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_source_.request_stop();
    }

    // then, delete the receiver to finish and disable all receiver-related callbacks
    receiver_.reset();

    // now, we can safely end the worker thread
    workers_exit_.store(true);

    worker_cond_.notify_all();
    if (worker_thread_.joinable())
    {
        worker_thread_.join();
    }

    // finally, explicitly close all the remaining sessions
    pid_session_map_.clear();
}

void MessagePassingServer::RunWorkerThread()
{
    constexpr std::int32_t kTimeoutInMs = 100;
    TimestampT t1 = TimestampT::clock::now() + std::chrono::milliseconds(kTimeoutInMs);

    std::unique_lock<std::mutex> lock(mutex_);
    while (!workers_exit_)
    {
        worker_cond_.wait_until(lock, t1, [this]() {
            return workers_exit_ || !work_queue_.empty();
        });
        if (!workers_exit_)
        {
            TimestampT now = TimestampT::clock::now();
            if (connection_timeout_ != TimestampT{} && now >= connection_timeout_)
            {
                connection_timeout_ = TimestampT{};
                stop_source_.request_stop();
            }
            if (now >= t1)
            {
                t1 = now + std::chrono::milliseconds(kTimeoutInMs);
                for (auto& ps : pid_session_map_)
                {
                    if (ps.second.GetIsSourceClosed())
                    {
                        /*
                            this is private functions so it cannot be test.
                        */
                        // LCOV_EXCL_START
                        ps.second.EnqueueForDeleteWhileLocked(true);
                        // LCOV_EXCL_STOP
                    }
                    else
                    {
                        auto& wrapper = ps.second;
                        if (wrapper.acquire_in_flight && wrapper.acquire_deadline.has_value() &&
                            (now >= *wrapper.acquire_deadline))
                        {
                            wrapper.acquire_in_flight = false;
                            ++wrapper.acquire_miss_count;
                            wrapper.acquire_deadline.reset();
                            if (wrapper.acquire_miss_count >= watchdog_config_.max_misses)
                            {
                                wrapper.EnqueueForDeleteWhileLocked(true);
                                continue;
                            }
                        }
                        ps.second.EnqueueTickWhileLocked();
                    }
                }
            }
        }

        while (!workers_exit_ && !work_queue_.empty())
        {
            pid_t pid = work_queue_.front();
            work_queue_.pop();
            SessionWrapper& wrapper = pid_session_map_.at(pid);
            wrapper.SetRunningWhileLocked();
            bool closed_by_peer = wrapper.GetResetClosedByPeer();
            lock.unlock();
            if (closed_by_peer)
            {
                wrapper.NotifyClosedByPeer();
            }
            bool requeue = wrapper.TickAtWorkerThread();
            lock.lock();
            if (wrapper.to_force_finish)
            {
                if (!closed_by_peer)
                {
                    // received to_force_finish_ for the session while ticking it;
                    // need to notify the ISession before continuing
                    /*
                        this is private functions so it cannot be test.
                    */
                    // LCOV_EXCL_START
                    wrapper.NotifyClosedByPeer();
                    requeue = true;
                    // LCOV_EXCL_STOP
                }
                if (requeue)
                {
                    // need to expedite finishing the ticks and erasing the map entry
                    // as the server thread is waiting to add another session with the same pid to the map
                    // LCOV_EXCL_START: see above
                    lock.unlock();
                    do
                    {
                        requeue = wrapper.TickAtWorkerThread();
                    } while (requeue);
                    lock.lock();
                    // LCOV_EXCL_STOP
                }
                // Extract the session wrapper to destroy it outside the mutex lock
                // to avoid deadlock when destructor blocks (e.g., ClientConnection::~ClientConnection)
                auto node = pid_session_map_.extract(pid);
                session_finishing_ = false;
                server_cond_.notify_all();
                lock.unlock();
                // Destroy the extracted node here (destructor runs without holding mutex)
                node = {};
                lock.lock();
            }
            else if (wrapper.ResetRunningWhileLocked(requeue))
            {
                // LCOV_EXCL_START: see above
                EnqueueTickWhileLocked(pid);
                // LCOV_EXCL_STOP
            }
            else if (wrapper.IsMarkedForDelete())
            {
                // Extract the session wrapper to destroy it outside the mutex lock
                auto node = pid_session_map_.extract(pid);
                lock.unlock();
                // Destroy the extracted node here (destructor runs without holding mutex)
                node = {};
                lock.lock();
            }
        }
    }
}

void MessagePassingServer::EnqueueTickWhileLocked(pid_t pid)
{
    bool was_empty = work_queue_.empty();
    work_queue_.push(pid);
    if (was_empty)
    {
        worker_cond_.notify_all();
    }
}

void MessagePassingServer::FinishPreviousSessionWhileLocked(
    std::unordered_map<pid_t, MessagePassingServer::SessionWrapper>::iterator it,
    std::unique_lock<std::mutex>& lock)
{
    const pid_t pid = it->first;
    SessionWrapper& wrapper = it->second;
    wrapper.to_force_finish = true;
    wrapper.EnqueueForDeleteWhileLocked(true);
    // if enqueued_ (i.e. not running) expedite the workload toward the front of the queue
    if (wrapper.enqueued)
    {
        pid_t front_pid = work_queue_.front();
        while (front_pid != pid)
        {
            /*
                this is private functions so it cannot be test.
            */
            // LCOV_EXCL_START
            work_queue_.pop();
            work_queue_.push(front_pid);
            front_pid = work_queue_.front();
            // LCOV_EXCL_STOP
        }
    }

    // we have only one server thread waiting on this condition (for only one session at a time)
    session_finishing_ = true;
    server_cond_.wait(lock, [this]() {
        return !session_finishing_;
    });
}

void MessagePassingServer::MessageCallback(score::message_passing::IServerConnection& connection,
                                           score::cpp::span<const std::uint8_t> message)
{
    const pid_t pid = connection.GetClientIdentity().pid;
    if (message.empty())
    {
        std::cerr << "MessagePassingServer: Empty message received from " << pid;
        return;
    }

    const auto message_type = message.front();
    const auto payload = message.subspan(1);
    switch (message_type)
    {
        case score::cpp::to_underlying(DatarouterMessageIdentifier::kConnect):
            OnConnectRequest(connection, payload, pid);
            break;
        case score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireResponse):
            OnAcquireResponse(connection, payload, pid);
            break;
        case score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireRequest):
            std::cerr << "MessagePassingServer: Unsupported Acquire Message received from " << pid;
            break;
        default:
            std::cerr << "MessagePassingServer: Unsupported MessageType received from " << pid;
            break;
    }
}

void MessagePassingServer::OnConnectRequest(score::message_passing::IServerConnection& connection,
                                            const score::cpp::span<const std::uint8_t> message,
                                            const pid_t pid)
{

    score::mw::log::detail::ConnectMessageFromClient conn;
    if (message.size() < sizeof(conn))
    {
        std::cout << "ConnectMessageFromClient too small" << std::endl;
        stop_source_ = score::cpp::stop_source();
        return;
    }
    /*
        Deviation from Rule M5-2-8:
        - Rule M5-2-8 (required, implementation, automated)
        An object with integer type or pointer to void type shall not be converted
        to an object with pointer type.
        Justification:
        - This is safe since we convert void conn object to it's raw form to fill it from message .
    */
    // coverity[autosar_cpp14_m5_2_8_violation]
    score::cpp::span<std::uint8_t> conn_span{static_cast<uint8_t*>(static_cast<void*>(&conn)), sizeof(conn)};
    std::ignore = std::copy_n(
        message.begin(), std::min(message.size(), static_cast<std::size_t>(conn_span.size())), conn_span.begin());

    auto appid_sv = conn.GetAppId().GetStringView();
    std::string appid{appid_sv.data(), appid_sv.size()};

    // check for timeout or exit request
    if (stop_source_.stop_requested())
    {
        std::cout << "Datarouter exits before connecting to client: " << appid << std::endl;
        // reset the source and return closing the (most likely inactive) sender
        stop_source_ = score::cpp::stop_source();
        return;
    }

    // Creating the session could potentially block on subscriber mutex, which
    // could already be locked by another thread. The potential dead lock
    // situation where one thread is blocked on the message passing server and
    // another thread is blocked on the subscriber mutex, is avoided by calling
    // the factory only with unlocked mutex.

    score::cpp::pmr::memory_resource* memory_resource = score::cpp::pmr::get_default_resource();
    ::score::cpp::pmr::unique_ptr<daemon::ISessionHandle> session_handle{
        ::score::cpp::pmr::make_unique<SessionHandle>(memory_resource, pid, this)};
    auto session = factory_(pid, conn, std::move(session_handle));
    if (session)
    {
    }
    else
    {
        /*
            this is private functions so it cannot be test.
        */
        // LCOV_EXCL_START
        std::cerr << "Fail to create session for pid: " << pid << std::endl;
        // LCOV_EXCL_STOP
    }

    if (session)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (disconnected_pids_.find(pid) != disconnected_pids_.end())
        {
            // Client disconnected before we could create/emplace the session.
            // Do not store &connection (may be already destroyed by the framework).
            return;
        }

        auto emplace_result = pid_session_map_.emplace(pid, SessionWrapper{this, pid, std::move(session)});
        if (!emplace_result.second)
        {
            // Existing session for this PID is still present; do not overwrite it.
            // The reconnect path is handled by disconnect_callback + worker-thread teardown.
            return;
        }

        // connection_ points to framework-owned object. It is cleared in disconnect_callback.
        emplace_result.first->second.connection = &connection;
        // enqueue the tick to speed up processing connection
        emplace_result.first->second.EnqueueTickWhileLocked();
    }
}

void MessagePassingServer::OnAcquireResponse(score::message_passing::IServerConnection& connection,
                                             const score::cpp::span<const std::uint8_t> message,
                                             const pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = pid_session_map_.find(pid);
    if (found != pid_session_map_.end())
    {
        auto& [key, session] = *found;
        std::ignore = key;
        if (session.connection != &connection)
        {
            return;
        }
        score::mw::log::detail::ReadAcquireResult acq{};
        /*
            Deviation from Rule M5-2-8:
            - Rule M5-2-8 (required, implementation, automated)
            An object with integer type or pointer to void type shall not be converted
            to an object with pointer type.
            Justification:
            - This is safe since we convert void acq_span object to it's raw form to fill it from message .
        */
        // coverity[autosar_cpp14_m5_2_8_violation]
        score::cpp::span<std::uint8_t> acq_span{static_cast<uint8_t*>(static_cast<void*>(&acq)), sizeof(acq)};
        std::ignore = std::copy_n(
            message.begin(), std::min(message.size(), static_cast<std::size_t>(acq_span.size())), acq_span.begin());
        session.session->OnAcquireResponse(acq);
        session.acquire_in_flight = false;
        session.acquire_deadline.reset();
        session.acquire_miss_count = 0U;
        // enqueue the tick to speed up processing acquire response
        session.EnqueueTickWhileLocked();
    }
}

bool MessagePassingServer::NotifyAcquireRequestWhileLocked(const pid_t pid)
{
    const auto found = pid_session_map_.find(pid);
    if (found == pid_session_map_.end())
    {
        return false;
    }

    auto& wrapper = found->second;
    if (wrapper.connection == nullptr)
    {
        return false;
    }

    if (wrapper.acquire_in_flight)
    {
        return true;
    }

    constexpr std::array<std::uint8_t, 1> kMessage{score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireRequest)};
    auto ret = wrapper.connection->Notify(kMessage);
    if (!ret)
    {
        // ENOBUFS indicates the notify pool is temporarily exhausted (a previous notification could still be in
        // flight). This is a transient condition - Let the watchdog will handle it if the client never responds.
        if (ret.error().GetOsDependentErrorCode() == ENOBUFS)
        {
            std::cerr << "MessagePassingServer: Notify pool exhausted for pid " << pid << ", skipping acquire request"
                      << std::endl;
            return true;
        }
        std::cerr << "MessagePassingServer: Notify failed for pid " << pid << ": " << ret.error() << std::endl;
        wrapper.EnqueueForDeleteWhileLocked(true);
    }
    else
    {
        wrapper.acquire_in_flight = true;
        wrapper.acquire_deadline = TimestampT::clock::now() + watchdog_config_.deadline;
    }
    return true;
}

bool MessagePassingServer::NotifyAcquireRequest(const pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return NotifyAcquireRequestWhileLocked(pid);
}

bool MessagePassingServer::SessionHandle::AcquireRequest() const
{
    if (server_ == nullptr)
    {
        return false;
    }
    return server_->NotifyAcquireRequest(pid_);
}

}  // namespace internal
}  // namespace platform
}  // namespace score
