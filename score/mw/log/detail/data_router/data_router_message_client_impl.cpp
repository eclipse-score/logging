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

#include "score/mw/log/detail/data_router/data_router_message_client_impl.h"

#include "score/assert.hpp"
#include "score/os/utils/signal.h"
#include "score/mw/log/detail/data_router/message_passing_config.h"
#include "score/mw/log/detail/error.h"
#include "score/mw/log/detail/initialization_reporter.h"

#include "score/os/utils/signal_impl.h"
#include "score/mw/log/detail/utils/signal_handling/signal_handling.h"
#include <array>
#include <thread>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

DatarouterMessageClientImpl::DatarouterMessageClientImpl(const MsgClientIdentifiers& ids,
                                                         MsgClientBackend backend,
                                                         MsgClientUtils utils,
                                                         const score::cpp::stop_source stop_source)
    : DatarouterMessageClient{},
      run_started_{},
      msg_client_ids_{ids},
      use_dynamic_datarouter_ids_{backend.IsUsingDynamicDatarouterIDs()},
      first_message_received_{false},
      utils_(std::move(utils)),
      unlinked_shared_memory_file_{false},
      shared_memory_writer_{backend.GetShMemWriter()},
      writer_file_name_{backend.GetWriterFilename()},
      message_passing_factory_{std::move(backend.GetMsgPassingFactory())},
      stop_source_{stop_source},
      sender_state_change_mutex_{},
      state_condition_{},
      sender_state_{},
      sender_{nullptr},
      receiver_{nullptr},
      connect_thread_{}
{
}

DatarouterMessageClientImpl::~DatarouterMessageClientImpl()
{
    DatarouterMessageClientImpl::Shutdown();  // Avoid virtual function calls in destructor (MISRA.DTOR.DYNAMIC)
}

void DatarouterMessageClientImpl::Run()
{
    // Macro for assertion is tolerated by decision.
    // coverity[autosar_cpp14_a15_4_2_violation]
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(run_started_ == false, "Run() must be called only once");
    run_started_ = true;
    SetupReceiver();
    RunConnectTask();
}

void DatarouterMessageClientImpl::RunConnectTask()
{
    // Since waiting for Datarouter to connect is a blocking operation we have to do this asynchronously.
    connect_thread_ = score::cpp::jthread([this]() noexcept {
        ConnectToDatarouter();
    });
}

void DatarouterMessageClientImpl::ConnectToDatarouter() noexcept
{
    BlockTermSignal();
    SetThreadName();

    if (!CreateSender().has_value())
    {
        ReportInitializationError(score::mw::log::detail::Error::kFailedToCreateMessagePassingClient,
                                  "Failed to create Message Passing Client.",
                                  msg_client_ids_.GetAppID().GetStringView());
        RequestInternalShutdown();
        return;
    }

    // Wait for the sender to be in Ready state before starting receiver
    {
        std::unique_lock<std::mutex> lock(sender_state_change_mutex_);
        state_condition_.wait(lock, [&stop_source = stop_source_, &sender_state = sender_state_]() {
            return (sender_state.has_value() &&
                    (sender_state.value() == score::message_passing::IClientConnection::State::kReady)) ||
                   stop_source.stop_requested();
        });
    }

    if (stop_source_.stop_requested())
    {
        RequestInternalShutdown();
        return;
    }

    if (!sender_state_.has_value() || (sender_state_.value() != score::message_passing::IClientConnection::State::kReady))
    {
        RequestInternalShutdown();
        return;
    }

    if (StartReceiver() == false)
    {
        RequestInternalShutdown();
        return;
    }

    CheckExitRequestAndSendConnectMessage();
}

void DatarouterMessageClientImpl::BlockTermSignal() const noexcept
{
    sigset_t sig_set{};

    auto result = utils_.GetSignal().SigEmptySet(sig_set);
    if (result.has_value() == false)
    {
        const auto underlying_error = result.error().ToStringContainer(result.error());
        ReportInitializationError(score::mw::log::detail::Error::kBlockingTerminationSignalFailed,
                                  underlying_error.data(),
                                  msg_client_ids_.GetAppID().GetStringView());
    }

    // signal handling is tolerated by design. Argumentation: Ticket-101432
    // coverity[autosar_cpp14_m18_7_1_violation]
    result = utils_.GetSignal().SigAddSet(sig_set, SIGTERM);
    if (result.has_value() == false)
    {
        const auto underlying_error = result.error().ToStringContainer(result.error());
        ReportInitializationError(score::mw::log::detail::Error::kBlockingTerminationSignalFailed,
                                  underlying_error.data(),
                                  msg_client_ids_.GetAppID().GetStringView());
    }

    /* NOLINTNEXTLINE(score-banned-function) using PthreadSigMask by design. Argumentation: Ticket-101432 */
    result = utils_.GetSignal().PthreadSigMask(SIG_BLOCK, sig_set);
    if (result.has_value() == false)
    {
        const auto underlying_error = result.error().ToStringContainer(result.error());
        ReportInitializationError(score::mw::log::detail::Error::kBlockingTerminationSignalFailed,
                                  underlying_error.data(),
                                  msg_client_ids_.GetAppID().GetStringView());
    }
}

void DatarouterMessageClientImpl::SetThreadName() noexcept
{
    constexpr auto kLoggerThreadName = "logger";
    const auto thread_id = utils_.GetPthread().self();
    const auto result = utils_.GetPthread().setname_np(thread_id, kLoggerThreadName);
    if (result.has_value() == false)
    {
        const auto error_details = result.error().ToStringContainer(result.error());
        ReportInitializationError(score::mw::log::detail::Error::kFailedToSetLoggerThreadName,
                                  std::string_view{error_details.data()},
                                  msg_client_ids_.GetAppID().GetStringView());
    }
}

void DatarouterMessageClientImpl::SetupReceiver() noexcept
{
    const score::message_passing::ServiceProtocolConfig service_protocol_config{
        msg_client_ids_.GetReceiverID(), MessagePassingConfig::kMaxMessageSize, 0U, 0U};

    constexpr score::message_passing::IServerFactory::ServerConfig kServerConfig{
        MessagePassingConfig::kMaxReceiverQueueSize, 0U, 0U};
    receiver_ = message_passing_factory_->CreateServer(service_protocol_config, kServerConfig);
}

bool DatarouterMessageClientImpl::StartReceiver()
{
    // When the receiver starts listening, receive callbacks may be called that use the sender to reply.
    // Thus we must create the sender before starting to listen to messages.
    // Note that the receiver callback may only be called after the connect task finished.
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(sender_ != nullptr, "The sender must be created before the receiver.");

    auto* this_ptr = this;
    auto connect_callback = [this_ptr](score::message_passing::IServerConnection& connection) noexcept -> std::uintptr_t {
        const auto result = SignalHandling::PThreadBlockSigTerm(this_ptr->utils_.GetSignal());
        const pid_t client_pid = connection.GetClientIdentity().pid;
        return static_cast<std::uintptr_t>(client_pid);
    };
    auto disconnect_callback = [this_ptr](score::message_passing::IServerConnection& /*connection*/) noexcept {
        this_ptr->RequestInternalShutdown();
    };
    auto received_send_message_callback = [this_ptr](
                                              score::message_passing::IServerConnection& /*connection*/,
                                              const score::cpp::span<const std::uint8_t> /*message*/) noexcept -> score::cpp::blank {
        this_ptr->OnAcquireRequest();
        return {};
    };
    auto received_send_message_with_reply_callback =
        [](score::message_passing::IServerConnection& /*connection*/,
           score::cpp::span<const std::uint8_t> /*message*/) noexcept -> score::cpp::blank {
        return {};
    };

    const auto result = receiver_->StartListening(connect_callback,
                                                  disconnect_callback,
                                                  received_send_message_callback,
                                                  received_send_message_with_reply_callback);

    if (result.has_value() == false)
    {
        const std::string underlying_error = result.error().ToString();
        ReportInitializationError(mw::log::detail::Error::kReceiverInitializationError,
                                  std::string_view{underlying_error},
                                  msg_client_ids_.GetAppID().GetStringView());

        std::array<std::string::value_type, 5> app_zero_terminated{};
        std::ignore = std::copy_n(msg_client_ids_.GetAppID().GetStringView().begin(),
                                  std::min(msg_client_ids_.GetAppID().GetStringView().size(),
                                           app_zero_terminated.size() - static_cast<std::size_t>(1)),
                                  app_zero_terminated.begin());
        std::cerr
            << "[[mw::log]] Application " << app_zero_terminated.data() << " (PID: " << msg_client_ids_.GetThisProcID()
            << ") failed to start message passing receiver. Please add the 'PROCMGR_AID_PATHSPACE' ability to your"
               "'app_config.json'."
            << '\n';

        return false;
    }
    return true;
}

void DatarouterMessageClientImpl::RequestInternalShutdown() noexcept
{
    // Unlink the shared memory file as early as possible to prevent memory leaks.
    UnlinkSharedMemoryFile();

    std::ignore = stop_source_.request_stop();
}

void DatarouterMessageClientImpl::CheckExitRequestAndSendConnectMessage() noexcept
{
    if (stop_source_.stop_requested())
    {
        ReportInitializationError(score::mw::log::detail::Error::kShutdownDuringInitialization,
                                  "Exit requested received before connecting to Datarouter.",
                                  msg_client_ids_.GetAppID().GetStringView());
        return;
    }
    SendConnectMessage();
}

void DatarouterMessageClientImpl::SendConnectMessage() noexcept
{
    ConnectMessageFromClient msg;
    msg.SetAppId(msg_client_ids_.GetAppID());
    msg.SetUid(msg_client_ids_.GetUID());
    msg.SetUseDynamicIdentifier(use_dynamic_datarouter_ids_);

    if (use_dynamic_datarouter_ids_ &&
        (writer_file_name_.size() >
         (MessagePassingConfig::kRandomFilenameStartIndex + msg.GetRandomPart().size() + static_cast<std::size_t>(1))))
    {
        auto start_iter = writer_file_name_.begin();
        std::advance(start_iter, static_cast<std::ptrdiff_t>(MessagePassingConfig::kRandomFilenameStartIndex));
        auto random_part = msg.GetRandomPart();
        std::ignore = std::copy_n(start_iter, random_part.size(), random_part.begin());
        msg.SetRandomPart(random_part);
    }

    const auto message = SerializeMessage(DatarouterMessageIdentifier::kConnect, msg);

    SendMessage(message);
}

/*
Deviation from Rule A15-5-3:
- A function declared with the noexcept exception specification shall not throw exceptions.
Justification:
- Shutdown is noexcept because it is called from the destructor and must not throw.
  All operations are either noexcept or guaranteed not to throw in practice. If an unexpected
  exception occurs during shutdown, std::terminate is acceptable behavior for a destructor context.
*/
// coverity[autosar_cpp14_a15_5_3_violation]
void DatarouterMessageClientImpl::Shutdown() noexcept
{
    if (not first_message_received_.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::ignore = stop_source_.request_stop();
    // Notify waiting threads in case they are waiting for state change
    state_condition_.notify_all();

    // Request the jthread to stop and wait for it to finish
    if (connect_thread_.joinable())
    {
        std::ignore = connect_thread_.request_stop();
        connect_thread_.join();
    }

    receiver_.reset();
    {
        std::unique_lock<std::mutex> lock(sender_mutex_);
        sender_.reset();
    }
    // Block until all pending tasks and threads have finished.
    UnlinkSharedMemoryFile();
}

score::cpp::expected_blank<score::os::Error> DatarouterMessageClientImpl::CreateSender() noexcept
{
    const score::message_passing::ServiceProtocolConfig protocol_config{
        MessagePassingConfig::kDatarouterReceiverIdentifier,
        MessagePassingConfig::kMaxMessageSize,
        MessagePassingConfig::kMaxReplySize,
        MessagePassingConfig::kMaxNotifySize};
    const score::message_passing::IClientFactory::ClientConfig client_config{0U, 10U, false, true, false};
    sender_ = message_passing_factory_->CreateClient(protocol_config, client_config);

    if (sender_ == nullptr)
    {
        std::cerr << "[[mw::log]] Application (PID: " << msg_client_ids_.GetThisProcID()
                  << ") failed to create Message Passing Client." << '\n';
        return score::cpp::make_unexpected(score::os::Error::createFromErrno(ENOMEM));
    }

    auto state_callback =
        [&state_mutex = sender_state_change_mutex_, &state = sender_state_, &condition = state_condition_](
            score::message_passing::IClientConnection::State new_state) noexcept {
            {
                std::lock_guard<std::mutex> callback_lock(state_mutex);
                state = new_state;
            }
            condition.notify_all();
        };

    sender_->Start(state_callback, score::message_passing::IClientConnection::NotifyCallback{});
    return {};
}

void DatarouterMessageClientImpl::OnAcquireRequest() noexcept
{
    // The acquire request shall be the first message Datarouter sends to the client.
    HandleFirstMessageReceived();

    // Acquire data and prepare the response.
    const auto acquire_result = shared_memory_writer_.ReadAcquire();
    const auto message = SerializeMessage(DatarouterMessageIdentifier::kAcquireResponse, acquire_result);

    SendMessage(message);
}

void DatarouterMessageClientImpl::HandleFirstMessageReceived() noexcept
{
    if (first_message_received_.load())
    {
        return;
    }
    first_message_received_ = true;

    UnlinkSharedMemoryFile();
}

void DatarouterMessageClientImpl::SendMessage(const score::cpp::span<const std::uint8_t>& message) noexcept
{
    score::cpp::expected_blank<score::os::Error> result;
    {
        std::unique_lock<std::mutex> lock(sender_mutex_);
        if (sender_ != nullptr)
        {
            result = sender_->Send(message);
        }
    }
    if (result.has_value() == false)
    {
        // The sender will retry sending the message for 10 s (retry_delay * number_of_retries).
        // If sending the message does not succeed despite all retries we assume Datarouter has crashed or is hanging
        // and consequently shutdown the logging in the client.
        // Send() already checks if the sender is in kReady state and returns EINVAL if not.

        const auto error_details = result.error().ToStringContainer(result.error());
        ReportInitializationError(score::mw::log::detail::Error::kFailedToSendMessageToDatarouter,
                                  std::string_view{error_details.data()},
                                  msg_client_ids_.GetAppID().GetStringView());

        RequestInternalShutdown();
    }
}

void DatarouterMessageClientImpl::UnlinkSharedMemoryFile() noexcept
{
    if (unlinked_shared_memory_file_)
    {
        return;
    }
    unlinked_shared_memory_file_ = true;
    const auto result = utils_.GetUnistd().unlink(writer_file_name_.c_str());
    if (result.has_value() == false)
    {
        const auto underlying_error = result.error().ToStringContainer(result.error());
        ReportInitializationError(
            Error::kUnlinkSharedMemoryError, underlying_error.data(), msg_client_ids_.GetAppID().GetStringView());
    }
}

const std::string& DatarouterMessageClientImpl::GetReceiverIdentifier() const noexcept
{
    return msg_client_ids_.GetReceiverID();
}

const pid_t& DatarouterMessageClientImpl::GetThisProcessPid() const noexcept
{
    return msg_client_ids_.GetThisProcID();
}

const std::string& DatarouterMessageClientImpl::GetWriterFileName() const noexcept
{
    return writer_file_name_;
}
const LoggingIdentifier& DatarouterMessageClientImpl::GetAppid() const noexcept
{
    return msg_client_ids_.GetAppID();
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
