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

#include "score/assert_support.hpp"
#include "score/message_passing/mock/client_connection_mock.h"
#include "score/message_passing/mock/server_connection_mock.h"
#include "score/message_passing/mock/server_mock.h"
#include "score/message_passing/server_types.h"
#include "score/os/mocklib/mock_pthread.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/os/utils/mocklib/signalmock.h"
#include "score/mw/log/detail/data_router/data_router_message_client_impl.h"
#include "score/mw/log/detail/data_router/message_passing_factory_mock.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <future>
#include <iostream>
namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
namespace
{

using ::testing::_;
using ::testing::ByMove;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::StrEq;

MATCHER_P(CompareServiceProtocol, expected, "")
{
    if (arg.identifier != expected.identifier || arg.max_send_size != expected.max_send_size ||
        arg.max_reply_size != expected.max_reply_size || arg.max_notify_size != expected.max_notify_size)
    {

        return false;
    }
    return true;
}

MATCHER_P(CompareServerConfig, expected, "")
{
    if (arg.max_queued_sends != expected.max_queued_sends ||
        arg.pre_alloc_connections != expected.pre_alloc_connections ||
        arg.max_queued_notifies != expected.max_queued_notifies)
    {
        return false;
    }
    return true;
}

MATCHER_P(CompareClientConfig, expected, "")
{
    if (arg.max_async_replies != expected.max_async_replies || arg.max_queued_sends != expected.max_queued_sends ||
        arg.fully_ordered != expected.fully_ordered || arg.truly_async != expected.truly_async)
    {
        return false;
    }
    return true;
}

const std::string_view kDatarouterReceiverIdentifier{"/logging.datarouter_recv"};
const std::string kClientReceiverIdentifier = "/logging.app.1234";
const auto kMwsrFileName = "/tmp" + kClientReceiverIdentifier + ".shmem";
const auto kAppid = LoggingIdentifier{"TeAp"};
const uid_t kUid = 1234;
const auto kDynamicDataRouterIdentifiers = true;
const pid_t kThisProcessPid = 99U;
constexpr pthread_t kThreadId = 42;
constexpr auto kLoggerThreadName = "logger";
constexpr uid_t kDatarouterDummyUid = 111;
constexpr std::uint32_t kMaxSendBytes{17U};
constexpr std::uint32_t kMaxNumberMessagesInReceiverQueue{0UL};

class DatarouterMessageClientFixture : public ::testing::Test
{
  public:
    DatarouterMessageClientFixture() = default;

    DatarouterMessageClientFixture(const bool dynamic_data_router_identifiers)
        : dynamic_data_router_identifiers_(dynamic_data_router_identifiers)
    {
    }

    DatarouterMessageClientFixture(const std::string mwsr_file_name) : mwsr_file_name_(mwsr_file_name) {}
    void SetUp() override
    {
        auto* memory_resource = score::cpp::pmr::get_default_resource();
        auto unistd_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::UnistdMock>>(memory_resource);
        unistd_mock_ = unistd_mock.get();
        auto pthread_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::MockPthread>>(memory_resource);
        pthread_mock_ = pthread_mock.get();
        auto signal_mock = score::cpp::pmr::make_unique<score::os::SignalMock>(memory_resource);
        signal_mock_ = signal_mock.get();
        auto message_passing_factory = std::make_unique<testing::StrictMock<MessagePassingFactoryMock>>();
        message_passing_factory_ = message_passing_factory.get();

        client_ = std::make_unique<DatarouterMessageClientImpl>(
            MsgClientIdentifiers(
                kClientReceiverIdentifier, kThisProcessPid, kAppid, static_cast<uid_t>(kDatarouterDummyUid), kUid),
            MsgClientBackend(shared_memory_writer_,
                             mwsr_file_name_,
                             std::move(message_passing_factory),
                             dynamic_data_router_identifiers_),
            MsgClientUtils{std::move(unistd_mock), std::move(pthread_mock), std::move(signal_mock)},
            stop_source_);
    }

    void TearDown() override
    {
        if (unlink_done_ == false)
        {
            // If not already done, the file should be unlinked on shutdown to prevent memory leaks.
            ExpectUnlinkMwsrWriterFile();
        }
    }

  protected:
    void ExpectBlockTerminationSignalPass()
    {
        EXPECT_CALL(*signal_mock_, SigEmptySet(_)).WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{}));
        EXPECT_CALL(*signal_mock_, SigAddSet(_, _)).WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{}));
        EXPECT_CALL(*signal_mock_, PthreadSigMask(_, _))
            .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{}));
    }

    void ExpectBlockTerminationSignalFail()
    {
        EXPECT_CALL(*signal_mock_, SigEmptySet(_))
            .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createUnspecifiedError())));
        EXPECT_CALL(*signal_mock_, SigAddSet(_, _))
            .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createUnspecifiedError())));
        EXPECT_CALL(*signal_mock_, PthreadSigMask(_, _))
            .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createUnspecifiedError())));
    }

    score::message_passing::ServerMock* ExpectReceiverCreated()
    {
        auto receiver = score::cpp::pmr::make_unique<testing::StrictMock<score::message_passing::ServerMock>>(
            score::cpp::pmr::get_default_resource());
        auto* receiver_ptr = receiver.get();
        const score::message_passing::ServiceProtocolConfig service_protocol_config{
            kClientReceiverIdentifier, kMaxSendBytes, 0U, 0U};

        const score::message_passing::IServerFactory::ServerConfig server_config{
            kMaxNumberMessagesInReceiverQueue, 0U, 0U};
        EXPECT_CALL(*message_passing_factory_,
                    CreateServer(CompareServiceProtocol(service_protocol_config), CompareServerConfig(server_config)))
            .WillOnce(Return(ByMove(std::move(receiver))));
        return receiver_ptr;
    }

    void ExpectReceiverStartListening(
        score::message_passing::ServerMock* receiver_ptr,
        score::message_passing::ConnectCallback* connect_callback = nullptr,
        score::message_passing::DisconnectCallback* disconnect_callback = nullptr,
        score::message_passing::MessageCallback* sent_callback = nullptr,
        score::message_passing::MessageCallback* sent_with_reply_callback = nullptr,
        score::cpp::expected_blank<score::os::Error> result = score::cpp::expected_blank<score::os::Error>{})
    {
        EXPECT_CALL(*receiver_ptr,
                    StartListening(Matcher<score::message_passing::ConnectCallback>(_),
                                   Matcher<score::message_passing::DisconnectCallback>(_),
                                   Matcher<score::message_passing::MessageCallback>(_),
                                   Matcher<score::message_passing::MessageCallback>(_)))
            .WillOnce([connect_callback, disconnect_callback, sent_callback, sent_with_reply_callback, result](
                          score::message_passing::ConnectCallback con_callback,
                          score::message_passing::DisconnectCallback discon_callback,
                          score::message_passing::MessageCallback sn_callback,
                          score::message_passing::MessageCallback sn_rep_callback) {
                if (connect_callback != nullptr)
                {
                    *connect_callback = std::move(con_callback);
                }
                if (disconnect_callback != nullptr)
                {
                    *disconnect_callback = std::move(discon_callback);
                }
                if (sent_callback != nullptr)
                {
                    *sent_callback = std::move(sn_callback);
                }
                if (sent_with_reply_callback != nullptr)
                {
                    *sent_with_reply_callback = std::move(sn_rep_callback);
                }
                return result;
            });
    }

    score::message_passing::ClientConnectionMock* ExpectSenderCreation(
        score::message_passing::IClientConnection::StateCallback* state_callback = nullptr,
        std::promise<void>* callback_registered = nullptr)
    {
        sender_mock_in_transit_ =
            score::cpp::pmr::make_unique<testing::StrictMock<score::message_passing::ClientConnectionMock>>(
                score::cpp::pmr::get_default_resource());
        auto* sender_mock = sender_mock_in_transit_.get();
        const score::message_passing::ServiceProtocolConfig service_protocol_config{
            kDatarouterReceiverIdentifier, kMaxSendBytes, 0U, 0U};

        const score::message_passing::IClientFactory::ClientConfig client_config{0, 10, false, true, false};

        EXPECT_CALL(*message_passing_factory_,
                    CreateClient(CompareServiceProtocol(service_protocol_config), CompareClientConfig(client_config)))
            .WillOnce(Return(ByMove(std::move(sender_mock_in_transit_))));

        EXPECT_CALL(*sender_mock,
                    Start(Matcher<score::message_passing::IClientConnection::StateCallback>(_),
                          Matcher<score::message_passing::IClientConnection::NotifyCallback>(_)))
            .WillOnce([state_callback, callback_registered](
                          score::message_passing::IClientConnection::StateCallback st_callback,
                          score::message_passing::IClientConnection::NotifyCallback) {
                if (state_callback != nullptr)
                {
                    *state_callback = std::move(st_callback);
                }
                if (callback_registered != nullptr)
                {
                    callback_registered->set_value();
                }
            });

        return sender_mock;
    }

    void ExpectClientDestruction(score::message_passing::ClientConnectionMock* sender_mock)
    {
        EXPECT_CALL(*sender_mock, Destruct());
    }

    void ExpectServerDestruction(score::message_passing::ServerMock* receiver_mock)
    {
        EXPECT_CALL(*receiver_mock, Destruct());
    }
    void ExpectSendAcquireResponse(score::message_passing::ClientConnectionMock* sender_ptr,
                                   ReadAcquireResult expected_content,
                                   score::cpp::expected_blank<score::os::Error> result = score::cpp::expected_blank<score::os::Error>{})
    {
        EXPECT_CALL(*sender_ptr, Send(Matcher<score::cpp::span<const std::uint8_t>>(_)))
            .WillOnce(
                [expected_content, result](score::cpp::span<const std::uint8_t> msg) -> score::cpp::expected_blank<score::os::Error> {
                    EXPECT_EQ(msg.front(), score::cpp::to_underlying(DatarouterMessageIdentifier::kAcquireResponse));
                    ReadAcquireResult data;
                    const auto payload = msg.subspan(1);

                    memcpy(&data, payload.data(), sizeof(data));
                    EXPECT_EQ(data.acquired_buffer, expected_content.acquired_buffer);
                    return result;
                });
    }

    void ExpectUnlinkMwsrWriterFile(bool unlink_successful = true)
    {
        EXPECT_CALL(*unistd_mock_, unlink(StrEq(mwsr_file_name_)))
            .WillOnce([unlink_successful](const char*) -> score::cpp::expected_blank<score::os::Error> {
                if (unlink_successful)
                {
                    return {};
                }
                else
                {
                    return score::cpp::make_unexpected(score::os::Error::createFromErrno());
                }
            });
        unlink_done_ = true;
    }

    void ExpectChownMwsrWriterForFile(bool datarouter_successful = true)
    {
        testing::InSequence order_matters;

        EXPECT_CALL(*unistd_mock_, chown(_, _, _))
            .WillOnce([datarouter_successful](const char*, uid_t, gid_t) -> score::cpp::expected_blank<score::os::Error> {
                if (datarouter_successful)
                {
                    return {};
                }
                else
                {
                    return score::cpp::make_unexpected(score::os::Error::createFromErrno());
                }
            });
    }

    void SendAcquireRequestAndExpectResponse(score::message_passing::MessageCallback& acquire_callback,
                                             score::message_passing::ClientConnectionMock** sender_ptr,
                                             bool first_message,
                                             bool unlink_successful = true)
    {
        ReadAcquireResult acquired_data{};
        acquired_data.acquired_buffer = shared_data_.control_block.switch_count_points_active_for_writing.load();

        if (first_message)
        {
            // MwsrWriter file shall be unlinked on first acquire request.
            ExpectUnlinkMwsrWriterFile(unlink_successful);
        }

        ExpectSendAcquireResponse(*sender_ptr, acquired_data);

        score::message_passing::ServerConnectionMock connection;
        const score::cpp::span<const std::uint8_t> message{};
        acquire_callback(connection, message);
    }

    void ExpectSenderAndReceiverCreation(
        score::message_passing::ServerMock** receiver_ptr,
        score::message_passing::ClientConnectionMock** sender_ptr,

        score::message_passing::IClientConnection::StateCallback* state_callback = nullptr,
        std::promise<void>* callback_registered = nullptr,
        score::cpp::expected_blank<score::os::Error> listen_result = score::cpp::expected_blank<score::os::Error>{},
        score::message_passing::ConnectCallback* connect_callback = nullptr,
        score::message_passing::DisconnectCallback* disconnect_callback = nullptr,
        score::message_passing::MessageCallback* sent_callback = nullptr,
        score::message_passing::MessageCallback* sent_with_reply_callback = nullptr,
        bool block_termination_signal_pass = true,
        bool receiver_start_listening = true)

    {
        std::ignore = block_termination_signal_pass;  // TODO: remove this param
        auto* receiver = ExpectReceiverCreated();
        if (receiver_ptr != nullptr)
        {
            *receiver_ptr = receiver;
        }

        if (block_termination_signal_pass)
        {
            ExpectBlockTerminationSignalPass();
        }
        else
        {
            ExpectBlockTerminationSignalFail();
        }

        ExpectSetLoggerThreadName();

        auto* sender = ExpectSenderCreation(state_callback, callback_registered);
        if (sender_ptr != nullptr)
        {
            *sender_ptr = sender;
        }
        if (receiver_start_listening)
        {
            ExpectReceiverStartListening(receiver,
                                         connect_callback,
                                         disconnect_callback,
                                         sent_callback,
                                         sent_with_reply_callback,
                                         listen_result);
        }
    }

    void ExpectSendConnectMessage(score::message_passing::ClientConnectionMock* sender_ptr)
    {
        EXPECT_CALL(*sender_ptr, Send(Matcher<score::cpp::span<const std::uint8_t>>(_)))
            .WillOnce([this](score::cpp::span<const std::uint8_t> msg) -> score::cpp::expected_blank<score::os::Error> {
                EXPECT_EQ(msg.front(), score::cpp::to_underlying(DatarouterMessageIdentifier::kConnect));
                std::array<char, 6> random_part;
                if (dynamic_data_router_identifiers_ && !mwsr_file_name_.empty())
                {
                    memcpy(random_part.data(), &mwsr_file_name_[13], random_part.size());
                }
                else
                {
                    random_part = {};
                }

                ConnectMessageFromClient expected_msg(kAppid, kUid, dynamic_data_router_identifiers_, random_part);
                ConnectMessageFromClient received_msg;
                const auto payload = msg.subspan(1);
                memcpy(&received_msg, payload.data(), sizeof(ConnectMessageFromClient));
                EXPECT_EQ(expected_msg, received_msg);
                return {};
            });
    }

    void ExpectSetLoggerThreadName(bool success = true)
    {
        EXPECT_CALL(*pthread_mock_, self).WillOnce(Return(kThreadId));
        auto setname_result = score::cpp::expected_blank<score::os::Error>{};
        if (success == false)
        {
            setname_result = score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno());
        }
        EXPECT_CALL(*pthread_mock_, setname_np(kThreadId, StrEq(kLoggerThreadName))).WillOnce(Return(setname_result));
    }

    void ExecuteCreateSenderAndReceiverSequence(
        bool expect_receiver_success = true,
        score::message_passing::IClientConnection::StateCallback* state_callback = nullptr)
    {
        client_->SetupReceiver();
        client_->BlockTermSignal();
        client_->SetThreadName();
        client_->CreateSender();
        (*state_callback)(score::message_passing::IClientConnection::State::kReady);
        EXPECT_EQ(client_->StartReceiver(), expect_receiver_success);
    }

    bool unlink_done_{false};
    bool dynamic_data_router_identifiers_{kDynamicDataRouterIdentifiers};
    std::string mwsr_file_name_{kMwsrFileName};

    score::cpp::pmr::unique_ptr<testing::StrictMock<score::message_passing::ClientConnectionMock>> sender_mock_in_transit_;

    testing::StrictMock<score::os::UnistdMock>* unistd_mock_;
    testing::StrictMock<score::os::MockPthread>* pthread_mock_;
    // TODO: bring back testing signal calls:
    score::os::SignalMock* signal_mock_;

    SharedData shared_data_{};
    SharedMemoryWriter shared_memory_writer_{InitializeSharedData(shared_data_), []() noexcept {}};

    std::unique_ptr<DatarouterMessageClientImpl> client_;
    testing::StrictMock<MessagePassingFactoryMock>* message_passing_factory_;
    score::cpp::stop_source stop_source_;
};

// Use this fixture if DynamicDataRouterIdentifiers should be disabled.
// That case random part will be empty in client message.
class DynamicDataRouterIdentifiersFalseFixture : public DatarouterMessageClientFixture
{
  public:
    DynamicDataRouterIdentifiersFalseFixture() : DatarouterMessageClientFixture(false) {}
};

// Use this fixture if Mswr file name should be empty.
// That case random part will be empty in client message.
class MwsrFileNameEmptyFixture : public DatarouterMessageClientFixture
{
  public:
    MwsrFileNameEmptyFixture() : DatarouterMessageClientFixture(std::string()) {}
};

TEST_F(DatarouterMessageClientFixture, CreateSenderShouldCreateSenderWithExpectedValues)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies creating the sender works properly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;
    auto* sender = ExpectSenderCreation();
    ExpectClientDestruction(sender);
    client_->CreateSender();
}

TEST_F(DatarouterMessageClientFixture, StartReceiverShouldStartListenSuccessfully)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies creating the receiver works properly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::IClientConnection::StateCallback state_callback;

    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, &state_callback);

    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
    ExecuteCreateSenderAndReceiverSequence(true, &state_callback);
}

TEST_F(DatarouterMessageClientFixture, StartReceiverWithoutSenderAndReceiverShouldFail)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies creating the receiver works properly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    EXPECT_DEATH(client_->StartReceiver(), "");
}
TEST_F(DatarouterMessageClientFixture, ReceiverStartListeningFailsShouldBeHandledGracefully)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of handling the receiver listen failure properly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::IClientConnection::StateCallback state_callback;

    auto start_listening_error = score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno());
    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, &state_callback, nullptr, start_listening_error);

    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
    ExecuteCreateSenderAndReceiverSequence(false, &state_callback);
}

TEST_F(DatarouterMessageClientFixture, SendConnectMessageShouldSendExpectedPayload)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'SendConnectMessage' API will send the expected payload.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    score::message_passing::IClientConnection::StateCallback state_callback;
    testing::InSequence order_matters;

    auto* sender = ExpectSenderCreation(&state_callback);
    ExpectSendConnectMessage(sender);
    ExpectClientDestruction(sender);

    client_->CreateSender();
    state_callback(score::message_passing::IClientConnection::State::kReady);
    client_->SendConnectMessage();
}

TEST_F(DynamicDataRouterIdentifiersFalseFixture,
       SendConnectMessageDynamicDataRouterIdentifiersFalseShouldSendExpectedPayload)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'SendConnectMessage' API will send the expected payload.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    score::message_passing::IClientConnection::StateCallback state_callback;
    testing::InSequence order_matters;

    auto* sender = ExpectSenderCreation(&state_callback);
    ExpectSendConnectMessage(sender);
    ExpectClientDestruction(sender);

    client_->CreateSender();
    state_callback(score::message_passing::IClientConnection::State::kReady);
    client_->SendConnectMessage();
}

TEST_F(MwsrFileNameEmptyFixture, SendConnectMessageMwsrFileNameEmptyShouldSendExpectedPayload)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'SendConnectMessage' API will send the expected payload.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    score::message_passing::IClientConnection::StateCallback state_callback;
    testing::InSequence order_matters;

    auto* sender = ExpectSenderCreation(&state_callback);
    ExpectSendConnectMessage(sender);
    ExpectClientDestruction(sender);

    client_->CreateSender();
    state_callback(score::message_passing::IClientConnection::State::kReady);
    client_->SendConnectMessage();
}

TEST_F(DatarouterMessageClientFixture, ConnectToDatarouterShouldSendConnectMessage)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of sending connect message when connecting to datarouter.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");
    score::message_passing::IClientConnection::StateCallback state_callback;
    std::promise<void> callback_registered;

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};

    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, &state_callback, &callback_registered);
    ExpectSendConnectMessage(sender_ptr);
    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);

    client_->SetupReceiver();
    // We need to unblock waiting for the connection so we change the state in a separate thread
    std::thread connect_thread([this]() noexcept {
        client_->ConnectToDatarouter();
    });
    callback_registered.get_future().wait();

    state_callback(score::message_passing::IClientConnection::State::kReady);

    connect_thread.join();
}

TEST_F(DatarouterMessageClientFixture, ConnectToDatarouterGivenThatReceiverFailedShouldNotSendConnectMessage)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of sending connect message when receiver fails.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::IClientConnection::StateCallback state_callback;
    std::promise<void> callback_registered;

    ExpectSenderAndReceiverCreation(&receiver_ptr,
                                    &sender_ptr,
                                    &state_callback,
                                    &callback_registered,
                                    score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno()));

    ExpectUnlinkMwsrWriterFile();

    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
    client_->SetupReceiver();
    // We need to unblock waiting for the connection so we change the state in a separate thread
    std::thread connect_thread([this]() noexcept {
        client_->ConnectToDatarouter();
    });

    callback_registered.get_future().wait();
    state_callback(score::message_passing::IClientConnection::State::kReady);

    connect_thread.join();
}

TEST_F(DatarouterMessageClientFixture, AcquireRequestShouldSendExpectedAcquireResponse)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that acquired request shall send the expected acquired response.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::ConnectCallback connect_callback;
    score::message_passing::DisconnectCallback disconnect_callback;
    score::message_passing::MessageCallback sent_callback;
    score::message_passing::MessageCallback sent_with_reply_callback;
    score::message_passing::IClientConnection::StateCallback state_callback;

    ExpectSenderAndReceiverCreation(&receiver_ptr,
                                    &sender_ptr,
                                    &state_callback,
                                    nullptr,
                                    {},
                                    &connect_callback,
                                    &disconnect_callback,
                                    &sent_callback,
                                    &sent_with_reply_callback);

    ExecuteCreateSenderAndReceiverSequence(true, &state_callback);

    bool first_message = true;
    SendAcquireRequestAndExpectResponse(sent_callback, &sender_ptr, first_message, false);
    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
}

TEST_F(DatarouterMessageClientFixture, SecondAcquireRequestShouldNotSetMwsrReader)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that the second acquire request should not set mwsr reader.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::ConnectCallback connect_callback;
    score::message_passing::DisconnectCallback disconnect_callback;
    score::message_passing::MessageCallback sent_callback;
    score::message_passing::MessageCallback sent_with_reply_callback;
    score::message_passing::IClientConnection::StateCallback state_callback;

    ExpectSenderAndReceiverCreation(&receiver_ptr,
                                    &sender_ptr,
                                    &state_callback,
                                    nullptr,
                                    {},
                                    &connect_callback,
                                    &disconnect_callback,
                                    &sent_callback,
                                    &sent_with_reply_callback);

    ExecuteCreateSenderAndReceiverSequence(true, &state_callback);

    bool first_message = true;
    SendAcquireRequestAndExpectResponse(sent_callback, &sender_ptr, first_message, false);

    first_message = false;
    SendAcquireRequestAndExpectResponse(sent_callback, &sender_ptr, first_message);
    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
}

// Refactor to acquire request
TEST_F(DatarouterMessageClientFixture, ClientShouldShutdownAfterFailingToSendMessage)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that the client should shutting down after failing to send message.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::ConnectCallback connect_callback;
    score::message_passing::DisconnectCallback disconnect_callback;
    score::message_passing::MessageCallback sent_callback;
    score::message_passing::MessageCallback sent_with_reply_callback;
    score::message_passing::IClientConnection::StateCallback state_callback;
    ExpectSenderAndReceiverCreation(&receiver_ptr,
                                    &sender_ptr,
                                    &state_callback,
                                    nullptr,
                                    {},
                                    &connect_callback,
                                    &disconnect_callback,
                                    &sent_callback,
                                    &sent_with_reply_callback);

    ExecuteCreateSenderAndReceiverSequence(true, &state_callback);

    auto send_error = score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno());

    ReadAcquireResult result{};
    result.acquired_buffer = shared_data_.control_block.switch_count_points_active_for_writing.load();

    ExpectUnlinkMwsrWriterFile();
    ExpectSendAcquireResponse(sender_ptr, result, send_error);

    score::message_passing::ServerConnectionMock connection;
    const score::cpp::span<const std::uint8_t> message{};
    sent_callback(connection, message);
    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
}

TEST_F(DatarouterMessageClientFixture, RunShouldSetupAndConnect)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'Run' API should setup the client and connect properly");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::IClientConnection::StateCallback state_callback;
    std::promise<void> callback_registered;

    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, &state_callback, &callback_registered);

    ExpectSendConnectMessage(sender_ptr);
    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
    ExpectUnlinkMwsrWriterFile();
    client_->Run();

    callback_registered.get_future().wait();

    state_callback(score::message_passing::IClientConnection::State::kReady);

    client_->Shutdown();
}

TEST_F(DatarouterMessageClientFixture, RunShallNotBeCalledMoreThanOnce)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of calling 'Run' API twice.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::IClientConnection::StateCallback state_callback;
    std::promise<void> callback_registered;

    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, &state_callback, &callback_registered);
    ExpectSendConnectMessage(sender_ptr);

    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
    ExpectUnlinkMwsrWriterFile();

    client_->Run();

    callback_registered.get_future().wait();

    state_callback(score::message_passing::IClientConnection::State::kReady);
    EXPECT_DEATH(client_->Run(), "");
}

TEST_F(DatarouterMessageClientFixture, SetThreadNameShouldSetLoggerThreadName)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that sets the thread name shall sets the logger thread name ");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    ExpectSetLoggerThreadName();
    client_->SetThreadName();
}

TEST_F(DatarouterMessageClientFixture, FailedSetThreadNameShouldBeHandledGracefully)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the ability of failing gracefully in case of in-ability of setting thread name.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    ExpectSetLoggerThreadName(false);
    client_->SetThreadName();
}

TEST_F(DatarouterMessageClientFixture, FailedToChownOwnMsrWriterFileForDataRouter)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of handling the failure of owning the msr file writer.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::IClientConnection::StateCallback state_callback;
    std::promise<void> callback_registered;

    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, &state_callback, &callback_registered);
    ExpectSendConnectMessage(sender_ptr);
    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
    ExpectUnlinkMwsrWriterFile(false);
    client_->Run();

    callback_registered.get_future().wait();
    state_callback(score::message_passing::IClientConnection::State::kReady);
    client_->Shutdown();
}

TEST_F(DatarouterMessageClientFixture, GivenExitRequestDuringConnectionShouldNotSendConnectMessage)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies that exit request during the connection shall not be able to send connect message.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};

    ExpectSenderAndReceiverCreation(&receiver_ptr,
                                    &sender_ptr,
                                    nullptr,
                                    nullptr,
                                    score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno()),
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    true,
                                    false);

    ExpectUnlinkMwsrWriterFile();
    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);
    client_->SetupReceiver();
    stop_source_.request_stop();
    client_->ConnectToDatarouter();
}

TEST_F(DatarouterMessageClientFixture, FailedToEmptySignalSet)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the case of in-ability of emptying a signal set.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::message_passing::ClientConnectionMock* sender_ptr{};
    score::message_passing::ServerMock* receiver_ptr{};
    score::message_passing::IClientConnection::StateCallback state_callback;
    std::promise<void> callback_registered;

    ExpectSenderAndReceiverCreation(&receiver_ptr,
                                    &sender_ptr,
                                    &state_callback,
                                    &callback_registered,
                                    score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno()),
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    nullptr,
                                    false);
    ExpectUnlinkMwsrWriterFile();

    ExpectServerDestruction(receiver_ptr);
    ExpectClientDestruction(sender_ptr);

    client_->SetupReceiver();
    // We need to unblock waiting for the connection so we change the state in a separate thread
    std::thread connect_thread([this]() noexcept {
        client_->ConnectToDatarouter();
    });
    callback_registered.get_future().wait();

    state_callback(score::message_passing::IClientConnection::State::kReady);
    connect_thread.join();
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
