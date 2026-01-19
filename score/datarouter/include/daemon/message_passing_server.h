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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_MESSAGE_PASSING_SERVER_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_MESSAGE_PASSING_SERVER_H

#include "score/mw/log/detail/data_router/data_router_messages.h"

#include "score/mw/log/detail/data_router/shared_memory/common.h"

#include "score/mw/com/message_passing/receiver_factory.h"
#include "score/mw/com/message_passing/sender_factory.h"
#include "score/mw/log/detail/logging_identifier.h"

#include "score/datarouter/daemon_communication/session_handle_interface.h"
#include <score/jthread.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace score
{
namespace platform
{
namespace internal
{

// MessagePassingServer interface for SessionWrapper
class IMessagePassingServerSessionWrapper
{
  public:
    virtual ~IMessagePassingServerSessionWrapper() = default;

    virtual void EnqueueTickWhileLocked(pid_t /*pid*/) = 0;
};

class MessagePassingServer : public IMessagePassingServerSessionWrapper
{
  public:
    class SessionHandle : public daemon::ISessionHandle
    {
      public:
        SessionHandle(pid_t pid,
                      MessagePassingServer* server,
                      score::cpp::pmr::unique_ptr<score::mw::com::message_passing::ISender> sender)
            : daemon::ISessionHandle(), sender_(std::move(sender)), pid_(pid), server_(server)
        {
        }

        void AcquireRequest() const override;

      private:
        score::cpp::pmr::unique_ptr<score::mw::com::message_passing::ISender> sender_;
        pid_t pid_;
        MessagePassingServer* server_;
    };

    class ISession
    {
      public:
        virtual bool tick() = 0;
        virtual void on_acquire_response(const score::mw::log::detail::ReadAcquireResult&) = 0;
        virtual void on_closed_by_peer() = 0;
        virtual bool is_source_closed() = 0;
        virtual ~ISession() = default;
    };

    using SessionFactory =
        std::function<std::unique_ptr<ISession>(const pid_t,
                                                const score::mw::log::detail::ConnectMessageFromClient&,
                                                score::cpp::pmr::unique_ptr<daemon::ISessionHandle>)>;

    MessagePassingServer(SessionFactory factory, ::score::concurrency::Executor& executor);
    ~MessagePassingServer() noexcept;

    // for unit test only. to keep rest of functions in private
    class MessagePassingServerForTest;

  private:
    static score::mw::com::message_passing::ShortMessage PrepareAcquireRequestMessage() noexcept;

    void NotifyAcquireRequestFailed(std::int32_t pid);

    void OnConnectRequest(const score::mw::com::message_passing::MediumMessagePayload payload, const pid_t pid);
    void OnAcquireResponse(const score::mw::com::message_passing::MediumMessagePayload payload, const pid_t pid);

    using timestamp_t = std::chrono::steady_clock::time_point;

    struct SessionWrapper
    {
        SessionWrapper(IMessagePassingServerSessionWrapper* server, pid_t pid, std::unique_ptr<ISession> session)
            : server_(server),
              pid_(pid),
              session_(std::move(session)),
              enqueued_(false),
              running_(false),
              to_delete_(false),
              closed_by_peer_(false),
              to_force_finish_(false)
        {
        }

        void enqueue_for_delete_while_locked(bool by_peer = false);
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

        bool tick_at_worker_thread();
        void notify_closed_by_peer();

        void set_running_while_locked();
        bool reset_running_while_locked(bool requeue);

        void enqueue_tick_while_locked();

        inline bool GetIsSourceClosed()
        {
            return session_->is_source_closed();
        }

        IMessagePassingServerSessionWrapper* server_;
        pid_t pid_;
        std::unique_ptr<ISession> session_;

        bool enqueued_;
        bool running_;
        bool to_delete_;
        bool closed_by_peer_;
        bool to_force_finish_;
    };

    void FinishPreviousSessionWhileLocked(std::unordered_map<pid_t, MessagePassingServer::SessionWrapper>::iterator it,
                                          std::unique_lock<std::mutex>& lock);
    void EnqueueTickWhileLocked(pid_t pid) override;
    void RunWorkerThread();

    SessionFactory factory_;

    score::cpp::pmr::unique_ptr<score::mw::com::message_passing::IReceiver> receiver_;

    std::mutex mutex_;
    score::cpp::stop_source stop_source_;
    timestamp_t connection_timeout_;
    score::cpp::jthread worker_thread_;
    std::condition_variable worker_cond_;  // to wake up worker thread
    std::unordered_map<pid_t, SessionWrapper> pid_session_map_;
    std::queue<pid_t> work_queue_;
    bool workers_exit_;
    std::condition_variable server_cond_;  // to wake up server thread
    bool session_finishing_;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_MESSAGE_PASSING_SERVER_H
