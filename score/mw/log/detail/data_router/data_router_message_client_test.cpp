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
#include "score/os/mocklib/mock_pthread.h"
#include "score/os/mocklib/unistdmock.h"
#include "score/os/utils/mocklib/signalmock.h"
#include "score/mw/com/message_passing/receiver_mock.h"
#include "score/mw/com/message_passing/sender_mock.h"
#include "score/mw/log/detail/data_router/data_router_message_client_impl.h"
#include "score/mw/log/detail/data_router/message_passing_factory_mock.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

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
using ::testing::An;
using ::testing::ByMove;
using ::testing::Eq;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrEq;

const std::string_view kDatarouterReceiverIdentifier{"/logging.datarouter_recv"};
const auto kAcquireRequestPayload = 0u;  // Arbitrary value since payload is ignored.
const auto kDatarouterPid = 324u;
const std::string kClientReceiverIdentifier = "/logging.app.1234";
const auto kMwsrFileName = "/tmp" + kClientReceiverIdentifier + ".shmem";
const auto kAppid = LoggingIdentifier{"TeAp"};
const uid_t kUid = 1234;
const auto kDynamicDataRouterIdentifiers = true;
const pid_t kThisProcessPid = 99u;
constexpr pthread_t kThreadId = 42;
constexpr auto kLoggerThreadName = "logger";
constexpr uid_t datarouter_dummy_uid = 111;

class DatarouterMessageClientFixture : public ::testing::Test
{
  public:
    DatarouterMessageClientFixture() = default;

    DatarouterMessageClientFixture(const bool dynamicDataRouterIdentifiers)
        : dynamicDataRouterIdentifiers_(dynamicDataRouterIdentifiers)
    {
    }

    DatarouterMessageClientFixture(const std::string mwsrFileName) : mwsrFileName_(mwsrFileName) {}
    void SetUp() override
    {
        auto memory_resource = score::cpp::pmr::get_default_resource();
        auto unistd_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::UnistdMock>>(memory_resource);
        unistd_mock_ = unistd_mock.get();
        auto pthread_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::MockPthread>>(memory_resource);
        pthread_mock_ = pthread_mock.get();
        auto signal_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::SignalMock>>(memory_resource);
        signal_mock_ = signal_mock.get();
        auto message_passing_factory = std::make_unique<testing::StrictMock<MessagePassingFactoryMock>>();
        message_passing_factory_ = message_passing_factory.get();

        client_ = std::make_unique<DatarouterMessageClientImpl>(
            MsgClientIdentifiers(
                kClientReceiverIdentifier, kThisProcessPid, kAppid, static_cast<uid_t>(datarouter_dummy_uid), kUid),
            MsgClientBackend(shared_memory_writer_,
                             mwsrFileName_,
                             std::move(message_passing_factory),
                             dynamicDataRouterIdentifiers_),
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

    score::mw::com::message_passing::ReceiverMock* ExpectReceiverCreated()
    {
        auto receiver = score::cpp::pmr::make_unique<testing::StrictMock<score::mw::com::message_passing::ReceiverMock>>(
            score::cpp::pmr::get_default_resource());
        auto receiver_ptr = receiver.get();
        EXPECT_CALL(*message_passing_factory_,
                    CreateReceiver(Eq(std::string_view{kClientReceiverIdentifier}), _, _, _, _))
            .WillOnce(Return(ByMove(std::move(receiver))));
        return receiver_ptr;
    }

    void ExpectReceiverRegisterCallbacks(
        score::mw::com::message_passing::ReceiverMock* receiver_ptr,
        score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback* acquire_callback = nullptr)
    {
        EXPECT_CALL(*receiver_ptr,
                    Register(static_cast<std::int8_t>(DatarouterMessageIdentifier::kAcquireRequest),
                             Matcher<score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback>(_)))
            .WillOnce(
                [acquire_callback](std::int8_t,
                                   score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback callback) {
                    if (acquire_callback != nullptr)
                    {
                        *acquire_callback = std::move(callback);
                    }
                });
    }

    void ExpectReceiverStartListening(
        score::mw::com::message_passing::ReceiverMock* receiver_ptr,
        score::cpp::expected_blank<score::os::Error> result = score::cpp::expected_blank<score::os::Error>{})
    {
        EXPECT_CALL(*receiver_ptr, StartListening()).WillOnce(Return(result));
    }

    score::mw::com::message_passing::SenderMock* ExpectSenderCreation()
    {
        // We need to implicitly transfer the ownership to the callback via the class member.
        // Necessary workaround because mutable&move capture callbacks are not accepted by gmock.
        // Since the callback should be called exactly once there are no memory errors.
        // If the code violates the expectation, the sanitizers and valgrind will detect memory errors.
        sender_mock_in_transit_ = score::cpp::pmr::make_unique<testing::StrictMock<score::mw::com::message_passing::SenderMock>>(
            score::cpp::pmr::get_default_resource());
        auto callback = [this](const std::string_view,
                               const score::cpp::stop_token&,
                               const score::mw::com::message_passing::SenderConfig&,
                               score::mw::com::message_passing::LoggingCallback log_callback,
                               score::cpp::pmr::memory_resource* /*resource*/)
            -> score::cpp::pmr::unique_ptr<score::mw::com::message_passing::ISender> {
            log_callback([](std::ostream&) {
                FAIL() << "expected to drop the logs";
            });
            return std::move(sender_mock_in_transit_);
        };
        EXPECT_CALL(*message_passing_factory_, CreateSender(Eq(kDatarouterReceiverIdentifier), _, _, _, _))
            .WillOnce(callback);
        return sender_mock_in_transit_.get();
    }

    void ExpectSendAcquireResponse(score::mw::com::message_passing::SenderMock* sender_ptr,
                                   ReadAcquireResult expected_content,
                                   score::cpp::expected_blank<score::os::Error> result = score::cpp::expected_blank<score::os::Error>{})
    {
        EXPECT_CALL(*sender_ptr, Send(Matcher<const score::mw::com::message_passing::MediumMessage&>(_)))
            .WillOnce([expected_content, result](const score::mw::com::message_passing::MediumMessage& msg)
                          -> score::cpp::expected_blank<score::os::Error> {
                EXPECT_EQ(msg.id, ToMessageId(DatarouterMessageIdentifier::kAcquireResponse));
                ReadAcquireResult data;
                memcpy(&data, msg.payload.data(), sizeof(data));
                EXPECT_EQ(data.acquired_buffer, expected_content.acquired_buffer);
                return result;
            });
    }

    void ExpectUnlinkMwsrWriterFile(bool unlink_successful = true)
    {
        EXPECT_CALL(*unistd_mock_, unlink(StrEq(mwsrFileName_)))
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

    void SendAcquireRequestAndExpectResponse(
        score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback& acquire_callback,
        score::mw::com::message_passing::SenderMock** sender_ptr,
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

        acquire_callback(kAcquireRequestPayload, kDatarouterPid);
    }

    void ExpectSenderAndReceiverCreation(
        score::mw::com::message_passing::ReceiverMock** receiver_ptr = nullptr,
        score::mw::com::message_passing::SenderMock** sender_ptr = nullptr,
        score::cpp::expected_blank<score::os::Error> listen_result = score::cpp::expected_blank<score::os::Error>{},
        score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback* acquire_callback = nullptr,
        bool blockTerminationSignalPass = true)
    {
        auto receiver = ExpectReceiverCreated();
        if (receiver_ptr != nullptr)
        {
            *receiver_ptr = receiver;
        }
        ExpectReceiverRegisterCallbacks(receiver, acquire_callback);

        if (blockTerminationSignalPass)
        {
            ExpectBlockTerminationSignalPass();
        }
        else
        {
            ExpectBlockTerminationSignalFail();
        }

        ExpectSetLoggerThreadName();

        auto sender = ExpectSenderCreation();
        if (sender_ptr != nullptr)
        {
            *sender_ptr = sender;
        }
        ExpectReceiverStartListening(receiver, listen_result);
    }

    void ExpectSendConnectMessage(score::mw::com::message_passing::SenderMock* sender_ptr)
    {
        EXPECT_CALL(*sender_ptr, Send(An<const score::mw::com::message_passing::MediumMessage&>()))
            .WillOnce(
                [this](const score::mw::com::message_passing::MediumMessage& msg) -> score::cpp::expected_blank<score::os::Error> {
                    EXPECT_EQ(msg.pid, kThisProcessPid);
                    EXPECT_EQ(msg.id, ToMessageId(DatarouterMessageIdentifier::kConnect));
                    std::array<char, 6> random_part;
                    if (dynamicDataRouterIdentifiers_ && !mwsrFileName_.empty())
                    {
                        memcpy(random_part.data(), &mwsrFileName_.data()[13], random_part.size());
                    }
                    else
                    {
                        random_part = {};
                    }

                    ConnectMessageFromClient expected_msg(kAppid, kUid, dynamicDataRouterIdentifiers_, random_part);
                    ConnectMessageFromClient received_msg;
                    memcpy(&received_msg, msg.payload.data(), sizeof(ConnectMessageFromClient));
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

    void ExecuteCreateSenderAndReceiverSequence(bool expect_receiver_success = true)
    {
        client_->SetupReceiver();
        client_->BlockTermSignal();
        client_->SetThreadName();
        client_->CreateSender();
        EXPECT_EQ(client_->StartReceiver(), expect_receiver_success);
    }

    bool unlink_done_{false};
    bool dynamicDataRouterIdentifiers_{kDynamicDataRouterIdentifiers};
    std::string mwsrFileName_{kMwsrFileName};

    score::cpp::pmr::unique_ptr<testing::StrictMock<score::mw::com::message_passing::SenderMock>> sender_mock_in_transit_;

    testing::StrictMock<score::os::UnistdMock>* unistd_mock_;
    testing::StrictMock<score::os::MockPthread>* pthread_mock_;
    testing::StrictMock<score::os::SignalMock>* signal_mock_;

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

TEST_F(DatarouterMessageClientFixture, SetupReceiverShouldRegisterExpectedCallbacks)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies that creating a distinct receiver will lead to registering of the proper callbacks.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;
    auto receiver = ExpectReceiverCreated();
    ExpectReceiverRegisterCallbacks(receiver);
    client_->SetupReceiver();
}

TEST_F(DatarouterMessageClientFixture, CreateSenderShouldCreateSenderWithExpectedValues)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies creating the sender works properly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;
    ExpectSenderCreation();
    client_->CreateSender();
}

TEST_F(DatarouterMessageClientFixture, StartReceiverShouldStartListenSuccessfully)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies creating the receiver works properly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    ExpectSenderAndReceiverCreation();
    ExecuteCreateSenderAndReceiverSequence();
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

    auto start_listening_error = score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno());
    ExpectSenderAndReceiverCreation(nullptr, nullptr, start_listening_error);

    ExecuteCreateSenderAndReceiverSequence(false);
}

TEST_F(DatarouterMessageClientFixture, SendConnectMessageShouldSendExpectedPayload)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'SendConnectMessage' API will send the expected payload.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    auto sender = ExpectSenderCreation();
    ExpectSendConnectMessage(sender);

    client_->CreateSender();
    client_->SendConnectMessage();
}

TEST_F(DynamicDataRouterIdentifiersFalseFixture,
       SendConnectMessageDynamicDataRouterIdentifiersFalseShouldSendExpectedPayload)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'SendConnectMessage' API will send the expected payload.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    auto sender = ExpectSenderCreation();
    ExpectSendConnectMessage(sender);

    client_->CreateSender();
    client_->SendConnectMessage();
}

TEST_F(MwsrFileNameEmptyFixture, SendConnectMessageMwsrFileNameEmptyShouldSendExpectedPayload)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'SendConnectMessage' API will send the expected payload.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    auto sender = ExpectSenderCreation();
    ExpectSendConnectMessage(sender);

    client_->CreateSender();
    client_->SendConnectMessage();
}

TEST_F(DatarouterMessageClientFixture, ConnectToDatarouterShouldSendConnectMessage)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of sending connect message when connecting to datarouter.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::mw::com::message_passing::SenderMock* sender_ptr{};

    ExpectSenderAndReceiverCreation(nullptr, &sender_ptr);
    ExpectSendConnectMessage(sender_ptr);

    client_->SetupReceiver();
    client_->ConnectToDatarouter();
}

TEST_F(DatarouterMessageClientFixture, ConnectToDatarouterGivenThatReceiverFailedShouldNotSendConnectMessage)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of sending connect message when receiver fails.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    ExpectSenderAndReceiverCreation(
        nullptr, nullptr, score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno()));
    ExpectUnlinkMwsrWriterFile();

    client_->SetupReceiver();
    client_->ConnectToDatarouter();
}

TEST_F(DatarouterMessageClientFixture, AcquireRequestShouldSendExpectedAcquireResponse)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that acquired request shall send the expected acquired response.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::mw::com::message_passing::SenderMock* sender_ptr{};
    score::mw::com::message_passing::ReceiverMock* receiver_ptr{};
    score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback acquire_callback;
    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, {}, &acquire_callback);

    ExecuteCreateSenderAndReceiverSequence();

    bool first_message = true;
    SendAcquireRequestAndExpectResponse(acquire_callback, &sender_ptr, first_message, false);
}

TEST_F(DatarouterMessageClientFixture, SecondAcquireRequestShouldNotSetMwsrReader)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that the second acquire request should not set mwsr reader.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::mw::com::message_passing::SenderMock* sender_ptr{};
    score::mw::com::message_passing::ReceiverMock* receiver_ptr{};
    score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback acquire_callback;
    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, {}, &acquire_callback);

    ExecuteCreateSenderAndReceiverSequence();

    bool first_message = true;
    SendAcquireRequestAndExpectResponse(acquire_callback, &sender_ptr, first_message, false);

    first_message = false;
    SendAcquireRequestAndExpectResponse(acquire_callback, &sender_ptr, first_message);
}

// Refactor to acquire request
TEST_F(DatarouterMessageClientFixture, ClientShouldShutdownAfterFailingToSendMessage)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that the client should shutting down after failing to send message.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::mw::com::message_passing::SenderMock* sender_ptr{};
    score::mw::com::message_passing::ReceiverMock* receiver_ptr{};
    score::mw::com::message_passing::IReceiver::ShortMessageReceivedCallback acquire_callback;
    ExpectSenderAndReceiverCreation(&receiver_ptr, &sender_ptr, {}, &acquire_callback);

    ExecuteCreateSenderAndReceiverSequence();

    auto send_error = score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno());

    ReadAcquireResult result{};
    result.acquired_buffer = shared_data_.control_block.switch_count_points_active_for_writing.load();

    ExpectUnlinkMwsrWriterFile();
    ExpectSendAcquireResponse(sender_ptr, result, send_error);

    acquire_callback(score::mw::com::message_passing::ShortMessagePayload{}, {});
}

TEST_F(DatarouterMessageClientFixture, RunShouldSetupAndConnect)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that 'Run' API should setup the client and connect properly");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;

    score::mw::com::message_passing::SenderMock* sender_ptr{};
    ExpectSenderAndReceiverCreation(nullptr, &sender_ptr);
    ExpectSendConnectMessage(sender_ptr);
    ExpectUnlinkMwsrWriterFile();
    client_->Run();
    client_->Shutdown();
}

TEST_F(DatarouterMessageClientFixture, RunShallNotBeCalledMoreThanOnce)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of calling 'Run' API twice.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    testing::InSequence order_matters;
    score::mw::com::message_passing::SenderMock* sender_ptr{};
    ExpectSenderAndReceiverCreation(nullptr, &sender_ptr);
    ExpectSendConnectMessage(sender_ptr);
    ExpectUnlinkMwsrWriterFile();

    client_->Run();

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

    score::mw::com::message_passing::SenderMock* sender_ptr{};
    ExpectSenderAndReceiverCreation(nullptr, &sender_ptr);
    ExpectSendConnectMessage(sender_ptr);
    ExpectUnlinkMwsrWriterFile(false);
    client_->Run();
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

    ExpectSenderAndReceiverCreation();
    ExpectUnlinkMwsrWriterFile();

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

    ExpectSenderAndReceiverCreation(
        nullptr, nullptr, score::cpp::make_unexpected<score::os::Error>(score::os::Error::createFromErrno()), nullptr, false);
    ExpectUnlinkMwsrWriterFile();

    client_->SetupReceiver();
    client_->ConnectToDatarouter();
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
