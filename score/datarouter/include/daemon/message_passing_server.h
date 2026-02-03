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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_MESSAGE_PASSING_SERVER_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_MESSAGE_PASSING_SERVER_H

#include "score/mw/log/detail/data_router/data_router_messages.h"

#include "score/mw/log/detail/data_router/shared_memory/common.h"

#include "score/message_passing/i_client_factory.h"
#include "score/message_passing/i_server_connection.h"
#include "score/message_passing/i_server_factory.h"
#include "score/mw/log/detail/logging_identifier.h"

#include "score/datarouter/daemon_communication/session_handle_interface.h"
#include <score/jthread.hpp>

#include "score/concurrency/interruptible_wait.h"
#include <score/stop_token.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
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

/// QNX message passing server for handling logging client connections.
///
/// Manages multiple client sessions and processes their log data asynchronously.
/// Uses a single worker thread to read from client shared memory buffers and route
/// log messages through the DataRouter pipeline.
///
/// Threading model:
/// - Dispatch thread: Created by QnxDispatchEngine; receives connection requests and
///   messages via QNX message passing (dispatch_block loop in QnxDispatchEngine::RunOnThread)
/// - Worker thread: Processes session tick events to read shared memory and route logs
///
/// Each client session is scheduled on the worker thread via a work queue to avoid
/// blocking the dispatch thread during potentially slow shared memory operations.
class MessagePassingServer : public IMessagePassingServerSessionWrapper
{
  public:
    class SessionHandle : public daemon::ISessionHandle
    {
      public:
        SessionHandle(pid_t pid,
                      MessagePassingServer* server,
                      score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> sender)
            : daemon::ISessionHandle(), sender_(std::move(sender)), pid_(pid), server_(server), sender_state_{}
        {
        }

        bool AcquireRequest() const override;

      private:
        score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> sender_;
        pid_t pid_;
        MessagePassingServer* server_;
        mutable std::optional<score::message_passing::IClientConnection::State> sender_state_;
    };

    class ISession
    {
      public:
        virtual bool Tick() = 0;
        virtual void OnAcquireResponse(const score::mw::log::detail::ReadAcquireResult&) = 0;
        virtual void OnClosedByPeer() = 0;
        virtual bool IsSourceClosed() = 0;
        virtual ~ISession() = default;
    };

    using SessionFactory =
        std::function<std::unique_ptr<ISession>(const pid_t,
                                                const score::mw::log::detail::ConnectMessageFromClient&,
                                                score::cpp::pmr::unique_ptr<daemon::ISessionHandle>)>;

    MessagePassingServer(SessionFactory factory,
                         std::shared_ptr<score::message_passing::IServerFactory> server_factory = nullptr,
                         std::shared_ptr<score::message_passing::IClientFactory> client_factory = nullptr);
    ~MessagePassingServer() noexcept;

    // for unit test only. to keep rest of functions in private
    class MessagePassingServerForTest;

  private:
    void NotifyAcquireRequestFailed(std::int32_t pid);

    void MessageCallback(const score::cpp::span<const std::uint8_t> message, const pid_t pid);
    void OnConnectRequest(const score::cpp::span<const std::uint8_t> message, const pid_t pid);
    void OnAcquireResponse(const score::cpp::span<const std::uint8_t> message, const pid_t pid);

    using TimestampT = std::chrono::steady_clock::time_point;

    struct SessionWrapper
    {
        SessionWrapper(IMessagePassingServerSessionWrapper* server_instance,
                       pid_t process_id,
                       std::unique_ptr<ISession> session_instance)
            : server(server_instance),
              pid(process_id),
              session(std::move(session_instance)),
              enqueued(false),
              running(false),
              to_delete(false),
              closed_by_peer(false),
              to_force_finish(false)
        {
        }

        void EnqueueForDeleteWhileLocked(bool by_peer = false);
        bool IsMarkedForDelete() const
        {
            return to_delete;
        }

        bool GetResetClosedByPeer()
        {
            bool by_peer = closed_by_peer;
            closed_by_peer = false;
            return by_peer;
        }

        bool TickAtWorkerThread() const;
        void NotifyClosedByPeer() const;

        void SetRunningWhileLocked();
        bool ResetRunningWhileLocked(bool requeue);

        void EnqueueTickWhileLocked();

        inline bool GetIsSourceClosed() const
        {
            return session->IsSourceClosed();
        }

        IMessagePassingServerSessionWrapper* server;
        pid_t pid;
        std::unique_ptr<ISession> session;

        bool enqueued;
        bool running;
        bool to_delete;
        bool closed_by_peer;
        bool to_force_finish;
    };

    void FinishPreviousSessionWhileLocked(std::unordered_map<pid_t, MessagePassingServer::SessionWrapper>::iterator it,
                                          std::unique_lock<std::mutex>& lock);
    void EnqueueTickWhileLocked(pid_t pid) override;
    void RunWorkerThread();

    SessionFactory factory_;

    score::cpp::pmr::unique_ptr<score::message_passing::IServer> receiver_;

    std::mutex mutex_;
    score::cpp::stop_source stop_source_;
    TimestampT connection_timeout_;
    score::cpp::jthread worker_thread_;
    std::condition_variable worker_cond_;  // to wake up worker thread
    std::unordered_map<pid_t, SessionWrapper> pid_session_map_;
    std::queue<pid_t> work_queue_;
    std::atomic<bool> workers_exit_;
    std::condition_variable server_cond_;  // to wake up server thread
    bool session_finishing_;

    std::shared_ptr<score::message_passing::IServerFactory> server_factory_;
    std::shared_ptr<score::message_passing::IClientFactory> client_factory_;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_MESSAGE_PASSING_SERVER_H
