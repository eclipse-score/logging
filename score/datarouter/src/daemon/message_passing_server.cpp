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

#include "score/memory.hpp"
#include <score/jthread.hpp>

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
using score::mw::log::detail::ToMessageId;

namespace
{

#if !defined(__QNXNTO__)

// In Linux, MQueue size is usually limited to 10 for non-privileged processes
std::int32_t kReceiverQueueMaxSize = 10;

#else

std::int32_t kReceiverQueueMaxSize = 128;

#endif

constexpr std::uint32_t kConnectionTimeoutInMs = 1000UL;
constexpr std::int32_t kMaxNumbersOfRetry = 5;
constexpr std::chrono::milliseconds kSendRetryDelay{};
constexpr std::chrono::milliseconds kConnectRetryDelay = std::chrono::milliseconds{5};
/*
    this functions is seen by this file only.
*/
// LCOV_EXCL_START
void DropLogs(const score::mw::com::message_passing::LogFunction) {}
// LCOV_EXCL_STOP

}  // namespace

void MessagePassingServer::SessionWrapper::enqueue_for_delete_while_locked(bool by_peer)
{
    to_delete_ = true;
    closed_by_peer_ = by_peer;
    // in order not to mess with the logic of the queue, we don't enqueue currently running tick. Instead, we mark it
    // to be deleted (or re-enqueued for post-mortem processing, if closed by peer) at the end of the tick processing
    if (!running_ && !enqueued_)
    {
        server_->EnqueueTickWhileLocked(pid_);
        enqueued_ = true;
    }
}

bool MessagePassingServer::SessionWrapper::tick_at_worker_thread()
{
    bool requeue = session_->tick();
    return requeue;
}

void MessagePassingServer::SessionWrapper::notify_closed_by_peer()
{
    session_->on_closed_by_peer();
}

void MessagePassingServer::SessionWrapper::set_running_while_locked()
{
    enqueued_ = false;
    running_ = true;
}

bool MessagePassingServer::SessionWrapper::reset_running_while_locked(bool requeue)
{
    running_ = false;
    // check if we need to re-enqueue the tick after running again. It may happen because:
    // 1. not all the work in tick was done (returned early to avoid congestion);
    // 2. the tick was marked for delete as "closed by peer" when running, but we don't expedite its finishing.
    if (requeue || closed_by_peer_)
    {
        enqueued_ = true;
    }
    return enqueued_;
}

void MessagePassingServer::SessionWrapper::enqueue_tick_while_locked()
{
    if (!enqueued_ && !to_delete_)
    {
        if (!running_)
        {
            server_->EnqueueTickWhileLocked(pid_);
        }
        enqueued_ = true;
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
                                           ::score::concurrency::Executor& executor)
    : factory_{std::move(factory)}, connection_timeout_{}, workers_exit_{false}, session_finishing_{false}
{
    worker_thread_ = score::cpp::jthread([this]() {
        RunWorkerThread();
    });

    auto ret_pthread = score::os::Pthread::instance().setname_np(worker_thread_.native_handle(), "mp_worker");
    if (!ret_pthread.has_value())
    {
        std::cerr << "setname_np: " << ret_pthread.error() << std::endl;
    }

    using namespace ::score::mw::com::message_passing;
    const std::string receiver_id{"/logging.datarouter_recv"};
    const std::vector<uid_t> allowed_uids{};
    score::mw::com::message_passing::ReceiverConfig receiver_config{};
    receiver_config.max_number_message_in_queue = kReceiverQueueMaxSize;
    receiver_ = ReceiverFactory::Create(receiver_id, executor, allowed_uids, receiver_config);

    receiver_->Register(ToMessageId(DatarouterMessageIdentifier::kConnect),
                        [this](const MediumMessagePayload payload, const pid_t pid) noexcept {
                            this->OnConnectRequest(payload, pid);
                        });
    receiver_->Register(ToMessageId(DatarouterMessageIdentifier::kAcquireResponse),
                        [this](const MediumMessagePayload payload, const pid_t pid) noexcept {
                            this->OnAcquireResponse(payload, pid);
                        });

    // all callbacks shall be registered before listening starts
    auto ret_listening = receiver_->StartListening();
    if (!ret_listening)
    {
        std::cerr << "StartListening: " << ret_listening.error() << std::endl;
    }
}

MessagePassingServer::~MessagePassingServer()
{
    // first, unblock the possible client connection requests
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_source_.request_stop();
    }

    // then, delete the receiver to finish and disable all receiver-related callbacks
    receiver_.reset();

    // now, we can safely end the worker thread
    {
        std::lock_guard<std::mutex> lock(mutex_);
        workers_exit_ = true;
    }
    worker_cond_.notify_all();
    worker_thread_.join();

    // finally, explicitly close all the remaining sessions
    pid_session_map_.clear();
}

void MessagePassingServer::RunWorkerThread()
{
    constexpr std::int32_t TIMEOUT_IN_MS = 100;
    timestamp_t t1 = timestamp_t::clock::now() + std::chrono::milliseconds(TIMEOUT_IN_MS);

    std::unique_lock<std::mutex> lock(mutex_);
    while (!workers_exit_)
    {
        worker_cond_.wait_until(lock, t1, [this]() {
            return workers_exit_ || !work_queue_.empty();
        });
        if (!workers_exit_)
        {
            timestamp_t now = timestamp_t::clock::now();
            if (connection_timeout_ != timestamp_t{} && now >= connection_timeout_)
            {
                connection_timeout_ = timestamp_t{};
                stop_source_.request_stop();
            }
            if (now >= t1)
            {
                t1 = now + std::chrono::milliseconds(TIMEOUT_IN_MS);
                for (auto& ps : pid_session_map_)
                {
                    if (ps.second.GetIsSourceClosed())
                    {
                        /*
                            this is private functions so it cannot be test.
                        */
                        // LCOV_EXCL_START
                        ps.second.enqueue_for_delete_while_locked(true);
                        // LCOV_EXCL_STOP
                    }
                    else
                    {
                        ps.second.enqueue_tick_while_locked();
                    }
                }
            }
        }

        while (!workers_exit_ && !work_queue_.empty())
        {
            pid_t pid = work_queue_.front();
            work_queue_.pop();
            SessionWrapper& wrapper = pid_session_map_.at(pid);
            wrapper.set_running_while_locked();
            bool closed_by_peer = wrapper.get_reset_closed_by_peer();
            lock.unlock();
            if (closed_by_peer)
            {
                wrapper.notify_closed_by_peer();
            }
            bool requeue = wrapper.tick_at_worker_thread();
            lock.lock();
            if (wrapper.to_force_finish_)
            {
                if (!closed_by_peer)
                {
                    // received to_force_finish_ for the session while ticking it;
                    // need to notify the ISession before continuing
                    /*
                        this is private functions so it cannot be test.
                    */
                    // LCOV_EXCL_START
                    wrapper.notify_closed_by_peer();
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
                        requeue = wrapper.tick_at_worker_thread();
                    } while (requeue);
                    lock.lock();
                    // LCOV_EXCL_STOP
                }
                pid_session_map_.erase(pid);
                session_finishing_ = false;
                server_cond_.notify_all();
            }
            else if (wrapper.reset_running_while_locked(requeue))
            {
                // LCOV_EXCL_START: see above
                EnqueueTickWhileLocked(pid);
                // LCOV_EXCL_STOP
            }
            else if (wrapper.is_marked_for_delete())
            {
                pid_session_map_.erase(pid);
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
    wrapper.to_force_finish_ = true;
    wrapper.enqueue_for_delete_while_locked(true);
    // if enqueued_ (i.e. not running) expedite the workload toward the front of the queue
    if (wrapper.enqueued_)
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

void MessagePassingServer::OnConnectRequest(const score::mw::com::message_passing::MediumMessagePayload payload,
                                            const pid_t pid)
{
    std::unique_lock<std::mutex> lock(mutex_);
    const auto found = pid_session_map_.find(pid);
    if (found != pid_session_map_.end())
    {
        // old pid owner died without us noticing, finish the old session
        FinishPreviousSessionWhileLocked(found, lock);
    }
    score::mw::log::detail::ConnectMessageFromClient conn;
    static_assert(payload.size() >= sizeof(conn), "ConnectMessageFromClient too big");
    /*
        Deviation from Rule M5-2-8:
        - Rule M5-2-8 (required, implementation, automated)
        An object with integer type or pointer to void type shall not be converted
        to an object with pointer type.
        Justification:
        - This is safe since we convert void conn object to it's raw form to fill it from payload .
    */
    // coverity[autosar_cpp14_m5_2_8_violation]
    score::cpp::span<std::uint8_t> conn_span{static_cast<uint8_t*>(static_cast<void*>(&conn)), sizeof(conn)};
    std::ignore = std::copy_n(payload.begin(), sizeof(conn), conn_span.begin());

    auto appid_sv = conn.GetAppId().GetStringView();
    std::string appid{appid_sv.data(), appid_sv.size()};
    // LCOV_EXCL_START: false positive since it is tested.
    std::string client_receiver_name;
    // LCOV_EXCL_STOP
    if (true == conn.GetUseDynamicIdentifier())
    {
        /*
            this is private functions so it cannot be test.
        */
        // LCOV_EXCL_START
        std::string random_part;
        for (const auto& s : conn.GetRandomPart())
        {
            random_part += s;
        }
        client_receiver_name = std::string("/logging-") + random_part;
        // LCOV_EXCL_STOP
    }
    else
    {
        client_receiver_name = std::string("/logging.") + appid + "." + std::to_string(conn.GetUid());
    }

    connection_timeout_ = timestamp_t::clock::now() + std::chrono::milliseconds(kConnectionTimeoutInMs);
    auto stop_token = stop_source_.get_token();

    lock.unlock();

    score::cpp::pmr::memory_resource* memory_resource = score::cpp::pmr::get_default_resource();

    constexpr score::mw::com::message_passing::SenderConfig sender_config{
        kMaxNumbersOfRetry, kSendRetryDelay, kConnectRetryDelay};
    auto sender = score::mw::com::message_passing::SenderFactory::Create(
        client_receiver_name, stop_token, sender_config, &DropLogs, memory_resource);

    lock.lock();

    connection_timeout_ = timestamp_t{};

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
    lock.unlock();

    ::score::cpp::pmr::unique_ptr<daemon::ISessionHandle> session_handle{
        ::score::cpp::pmr::make_unique<SessionHandle>(memory_resource, pid, this, std::move(sender))};
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
    lock.lock();

    if (session)
    {
        auto emplace_result = pid_session_map_.emplace(pid, SessionWrapper{this, pid, std::move(session)});
        // enqueue the tick to speed up processing connection
        emplace_result.first->second.enqueue_tick_while_locked();
    }
}

void MessagePassingServer::OnAcquireResponse(const score::mw::com::message_passing::MediumMessagePayload payload,
                                             const pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = pid_session_map_.find(pid);
    if (found != pid_session_map_.end())
    {
        score::mw::log::detail::ReadAcquireResult acq{};
        static_assert(payload.size() >= sizeof(acq), "MwsrDataReadAcquired too big");
        /*
            Deviation from Rule M5-2-8:
            - Rule M5-2-8 (required, implementation, automated)
            An object with integer type or pointer to void type shall not be converted
            to an object with pointer type.
            Justification:
            - This is safe since we convert void acq_span object to it's raw form to fill it from payload .
        */
        // coverity[autosar_cpp14_m5_2_8_violation]
        score::cpp::span<std::uint8_t> acq_span{static_cast<uint8_t*>(static_cast<void*>(&acq)), sizeof(acq)};
        std::ignore = std::copy_n(payload.begin(), sizeof(acq), acq_span.begin());
        found->second.session_->on_acquire_response(acq);
        // enqueue the tick to speed up processing acquire response
        found->second.enqueue_tick_while_locked();
    }
}

void MessagePassingServer::NotifyAcquireRequestFailed(std::int32_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto found = pid_session_map_.find(pid);
    if (found == pid_session_map_.end())
    {
        /*
         Code will be hit only in case of pid changed,
         but since this is private functions so it cannot be test.
        */
        // LCOV_EXCL_START
        return;
        // LCOV_EXCL_STOP
    }
    found->second.enqueue_for_delete_while_locked(true);
}
/*
    Deviation from Rule A3-1-1:
    - It shall be possible to include any header file
     in multiple translation units without violating the One Definition Rule.
    Justification:
    - This is false positive. Function is declared only once.
*/
// coverity[autosar_cpp14_a3_1_1_violation]
score::mw::com::message_passing::ShortMessage MessagePassingServer::PrepareAcquireRequestMessage() noexcept
{
    const auto my_pid = ::score::os::Unistd::instance().getpid();
    score::mw::com::message_passing::ShortMessage message{};
    message.id = ToMessageId(DatarouterMessageIdentifier::kAcquireRequest);
    message.pid = my_pid;  // the receiver will check if the pid matches the sending process
    message.payload = 0;
    return message;
}

void MessagePassingServer::SessionHandle::AcquireRequest() const
{
    const score::mw::com::message_passing::ShortMessage message = MessagePassingServer::PrepareAcquireRequestMessage();
    auto ret = sender_->Send(message);
    if (!ret)
    {
        if (server_ != nullptr)
        {
            server_->NotifyAcquireRequestFailed(pid_);
        }
    }
}

}  // namespace internal
}  // namespace platform
}  // namespace score
