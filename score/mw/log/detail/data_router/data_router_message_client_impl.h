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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_IMPL_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_IMPL_H

#include "score/expected.hpp"
#include "score/jthread.hpp"
#include "score/optional.hpp"
#include "score/stop_token.hpp"
#include "score/message_passing/i_server_connection.h"
#include "score/os/errno.h"
#include "score/mw/log/detail/data_router/data_router_message_client.h"
#include "score/mw/log/detail/data_router/data_router_message_client_backend.h"
#include "score/mw/log/detail/data_router/data_router_message_client_identifiers.h"
#include "score/mw/log/detail/data_router/data_router_message_client_utils.h"
#include "score/mw/log/detail/data_router/data_router_messages.h"

#include <condition_variable>
#include <mutex>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

constexpr std::size_t GetMonotonicResourceSize() noexcept
{
    return static_cast<std::size_t>(5) * static_cast<std::size_t>(1024);
}

/// \brief The concrete implementation of the DatarouterMessageClient.
/// \pre This class should not be used directly, but only through the abstract interface.
///
/// Remarks on thread safety:
/// The methods of the parent interface are not thread safe.
/// The Shutdown() method must only be called after the Run() method.
/// After calling Run() there shall be a single background thread
/// that delivers callbacks to the On*Request() methods.
class DatarouterMessageClientImpl : public DatarouterMessageClient
{
  public:
    explicit DatarouterMessageClientImpl(const MsgClientIdentifiers& ids,
                                         MsgClientBackend backend,
                                         MsgClientUtils utils,
                                         const score::cpp::stop_source stop_source = {});
    virtual ~DatarouterMessageClientImpl();
    DatarouterMessageClientImpl(DatarouterMessageClientImpl&&) = delete;
    DatarouterMessageClientImpl(const DatarouterMessageClientImpl&) = delete;
    DatarouterMessageClientImpl& operator=(DatarouterMessageClientImpl&&) = delete;
    DatarouterMessageClientImpl& operator=(const DatarouterMessageClientImpl&) = delete;

    /// \pre Shall be called only once.
    void Run() override;

    /// \pre Shall not be called concurrently to Run().
    void Shutdown() noexcept override;

    void SetupReceiver() noexcept;

    /// \brief Creates the message passing client (sender) for communication with Datarouter.
    /// \returns An empty expected on success, or an error if the sender could not be created.
    score::cpp::expected_blank<score::os::Error> CreateSender() noexcept;

    /// \pre SetupReceiver and CreateSender() called before.
    /// \returns true if the receiver was started successfully.
    bool StartReceiver();

    /// \pre CreateSender() called before.
    void SendConnectMessage() noexcept;

    /// \brief Sets the thread name of the logger thread.
    void SetThreadName() noexcept;

    /// \pre SetupReceiver() called before.
    void ConnectToDatarouter() noexcept;

    void BlockTermSignal() const noexcept;

    const std::string& GetReceiverIdentifier() const noexcept;
    const pid_t& GetThisProcessPid() const noexcept;
    const std::string& GetWriterFileName() const noexcept;
    const LoggingIdentifier& GetAppid() const noexcept;

  private:
    void RunConnectTask();
    void OnAcquireRequest() noexcept;
    void UnlinkSharedMemoryFile() noexcept;
    void HandleFirstMessageReceived() noexcept;
    void RequestInternalShutdown() noexcept;
    void CheckExitRequestAndSendConnectMessage() noexcept;

    void SendMessage(const score::cpp::span<const std::uint8_t>& message) noexcept;

    bool run_started_;

    MsgClientIdentifiers msg_client_ids_;
    bool use_dynamic_datarouter_ids_;

    std::atomic_bool first_message_received_;

    MsgClientUtils utils_;
    bool unlinked_shared_memory_file_;

    SharedMemoryWriter& shared_memory_writer_;
    //  score::platform::internal::MwsrWriter& mwsr_writer_;
    std::string writer_file_name_;
    std::unique_ptr<MessagePassingFactory> message_passing_factory_;

    score::cpp::stop_source stop_source_;

    mutable std::mutex sender_state_change_mutex_;
    mutable std::mutex sender_mutex_;
    std::condition_variable state_condition_;
    score::cpp::optional<score::message_passing::IClientConnection::State> sender_state_;

    // The construction/destruction order is critical here!
    // Sender and receiver both may contain running tasks.
    // Receiver tasks (callbacks) may use the sender.
    // Thus the receiver needs to destruct first, and then the sender.
    // Finally it is safe to join the connect thread.
    // Only then we can ensure that there are no concurrent tasks
    // accessing private data from another thread.
    score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> sender_;
    score::cpp::pmr::unique_ptr<score::message_passing::IServer> receiver_;
    score::cpp::jthread connect_thread_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_IMPL_H
