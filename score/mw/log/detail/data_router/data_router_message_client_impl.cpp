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
#include "score/mw/log/detail/data_router/data_router_messages.h"
#include "score/mw/log/detail/error.h"
#include "score/mw/log/detail/initialization_reporter.h"

#include <array>
#include <iostream>
#include <thread>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

using std::chrono_literals::operator"" ms;

namespace
{

constexpr std::size_t kNumberOfThread{1UL};
constexpr std::size_t kMaxNumberMessagesInReceiverQueue{1UL};
constexpr std::int32_t kSenderMaxNumberOfSendRetries{1000};
constexpr std::chrono::milliseconds kSenderRetrySendDelay = std::chrono::milliseconds(100);
constexpr std::chrono::milliseconds kSenderRetryConnectDelay = std::chrono::milliseconds(100);
constexpr std::chrono::milliseconds kReceiverMessageLoopDelay = std::chrono::milliseconds(10);
constexpr std::size_t kStartIndexOfRandomFileName{13UL};

template <typename Message, typename Payload>
auto BuildMessageImpl(const DatarouterMessageIdentifier id, const Payload& payload, const pid_t pid) -> Message
{
    Message message;
    message.payload = payload;
    message.id = ToMessageId(id);
    message.pid = pid;
    return message;
}

void SenderLoggerCallback(score::mw::com::message_passing::LogFunction) {}

}  // namespace

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
      monotonic_resource_buffer_{},
      monotonic_resource_{monotonic_resource_buffer_.data(),
                          monotonic_resource_buffer_.size(),
                          score::cpp::pmr::null_memory_resource()},
      stop_source_{stop_source},
      thread_pool_{kNumberOfThread, &monotonic_resource_},
      sender_{nullptr},
      receiver_{nullptr},
      connect_task_{}
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
    // clang-format off
    connect_task_ = thread_pool_.Submit(
        [this](const score::cpp::stop_token&) noexcept
        {
            ConnectToDatarouter();
        });
}

void DatarouterMessageClientImpl::ConnectToDatarouter() noexcept
{
    BlockTermSignal();

    SetThreadName();
    CreateSender();
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
    // Initialization of Array with client ids
    // coverity[autosar_cpp14_m8_5_2_violation]
    const std::array<uid_t, 1UL> allowed_uids{msg_client_ids_.GetDatarouterUID()};
    const score::mw::com::message_passing::ReceiverConfig receiver_config{
        static_cast<std::int32_t>(kMaxNumberMessagesInReceiverQueue), kReceiverMessageLoopDelay};
    receiver_ = message_passing_factory_->CreateReceiver(
        msg_client_ids_.GetReceiverID(), thread_pool_, allowed_uids, receiver_config, &monotonic_resource_);

    // clang-format off
    receiver_->Register(ToMessageId(DatarouterMessageIdentifier::kAcquireRequest),
                        [this](std::uint64_t, pid_t) noexcept
                        {
                            this->OnAcquireRequest();
                        });
}

bool DatarouterMessageClientImpl::StartReceiver()
{
    // When the receiver starts listening, receive callbacks may be called that use the sender to reply.
    // Thus we must create the sender before starting to listen to messages.
    // Note since the thread_pool_ shall only have one thread, the receiver callback may only be called after the
    // connect task finished.
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(sender_ != nullptr, "The sender must be created before the receiver.");

    const auto result = receiver_->StartListening();
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
    thread_pool_.Shutdown();
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
         (kStartIndexOfRandomFileName + msg.GetRandomPart().size() + static_cast<std::size_t>(1))))
    {
        auto start_iter = writer_file_name_.begin();
        std::advance(start_iter, static_cast<int>(kStartIndexOfRandomFileName));
        auto random_part = msg.GetRandomPart();
        std::ignore = std::copy_n(start_iter, random_part.size(), random_part.begin());
        msg.SetRandomPart(random_part);
    }

    score::mw::com::message_passing::MediumMessagePayload payload{};
    static_assert(sizeof(msg) <= payload.size(), "Connect message too large");
    static_assert(std::is_trivially_copyable<decltype(msg)>::value, "Message must be copyable");
    // Suppress "AUTOSAR C++14 M5-2-8" rule. The rule declares:
    // An object with integer type or pointer to void type shall not be converted to an object with pointer type.
    // But we need to convert void pointer to bytes for serialization purposes, no out of bounds there
    // coverity[autosar_cpp14_m5_2_8_violation]
    const score::cpp::span<const std::uint8_t> message_span{static_cast<const uint8_t*>(static_cast<const void*>(&msg)),
                                                     sizeof(msg)};
    // coverity[autosar_cpp14_m5_0_16_violation:FALSE]
    std::ignore = std::copy(message_span.begin(), message_span.end(), payload.begin());

    SendMessage(BuildMessage(DatarouterMessageIdentifier::kConnect, payload));
}

void DatarouterMessageClientImpl::Shutdown() noexcept
{
    if (not first_message_received_.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::ignore = stop_source_.request_stop();
    thread_pool_.Shutdown();
    if (connect_task_.Valid() == true)
    {
        connect_task_.Abort();
        std::ignore = connect_task_.Wait();
    }
    receiver_.reset();
    sender_.reset();
    // Block until all pending tasks and threads have finished.
    UnlinkSharedMemoryFile();
}

void DatarouterMessageClientImpl::CreateSender() noexcept
{
    // We are not using the stop token from the current task, because the sender shall be used even after the
    // current task exited. The connect task thus can only be stopped if the stop_source_ is triggered.
    const score::mw::com::message_passing::SenderConfig sender_config{
        kSenderMaxNumberOfSendRetries, kSenderRetrySendDelay, kSenderRetryConnectDelay};
    constexpr std::string_view kDatarouterReceiverIdentifier{"/logging.datarouter_recv"};
    sender_ = message_passing_factory_->CreateSender(kDatarouterReceiverIdentifier,
                                                     stop_source_.get_token(),
                                                     sender_config,
                                                     &SenderLoggerCallback,
                                                     &monotonic_resource_);

    // The creation of the sender is blocking until Datarouter is available.
    // Thus at this point, either Datarouter was available, or the stop_source_ was triggered.
    // The Sender interface currently does not expose the information if the Sender could be connected successfully to a
    // receiver.
}

void DatarouterMessageClientImpl::OnAcquireRequest() noexcept
{
    // The acquire request shall be the first message Datarouter sends to the client.
    HandleFirstMessageReceived();

    // Acquire data and prepare the response.
    const auto acquire_result = shared_memory_writer_.ReadAcquire();
    //  TODO: Use ShortMessagePayload instead:
    score::mw::com::message_passing::MediumMessagePayload payload{};
    static_assert(sizeof(payload) >= sizeof(acquire_result), "payload shall be large enough");
    const score::cpp::span<const std::uint8_t> acquire_result_span{
        // Suppress "AUTOSAR C++14 M5-2-8" rule. The rule declares:
        // An object with integer type or pointer to void type shall not be converted to an object with pointer type.
        // But we need to convert void pointer to bytes for serialization purposes, no out of bounds there
        // coverity[autosar_cpp14_m5_2_8_violation]
        static_cast<const uint8_t*>(static_cast<const void*>(&acquire_result)),
        sizeof(acquire_result)};
    // coverity[autosar_cpp14_m5_0_16_violation:FALSE]
    std::ignore = std::copy(acquire_result_span.begin(), acquire_result_span.end(), payload.begin());
    SendMessage(BuildMessage(DatarouterMessageIdentifier::kAcquireResponse, payload));
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

template <typename Message>
void DatarouterMessageClientImpl::SendMessage(const Message& message) noexcept
{
    auto result = sender_->Send(message);
    if (result.has_value() == false)
    {
        // The sender will retry sending the message for 10 s (retry_delay * number_of_retries).
        // If sending the message does not succeed despite all retries we assume Datarouter has crashed or is hanging
        // and consequently shutdown the logging in the client.

        const auto error_details = result.error().ToStringContainer(result.error());
        ReportInitializationError(score::mw::log::detail::Error::kFailedToSendMessageToDatarouter,
                                  std::string_view{error_details.data()},
                                  msg_client_ids_.GetAppID().GetStringView());

        RequestInternalShutdown();
    }
}

score::mw::com::message_passing::MediumMessage DatarouterMessageClientImpl::BuildMessage(
    const DatarouterMessageIdentifier& id,
    const score::mw::com::message_passing::MediumMessagePayload& payload) const noexcept
{
    return BuildMessageImpl<score::mw::com::message_passing::MediumMessage,
                            score::mw::com::message_passing::MediumMessagePayload>(
        id, payload, msg_client_ids_.GetThisProcID());
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
