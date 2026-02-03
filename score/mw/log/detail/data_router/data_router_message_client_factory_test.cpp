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
#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/detail/data_router/data_router_message_client_factory_impl.h"
#include "score/mw/log/detail/data_router/data_router_message_client_impl.h"
#include "score/mw/log/detail/data_router/message_passing_factory_mock.h"
#include "score/mw/log/legacy_non_verbose_api/tracing.h"

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
using ::testing::ByMove;
using ::testing::Eq;
using ::testing::Matcher;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::StrEq;

const auto kMwsrFileName = "";
constexpr pid_t kThisProcessPid = 1234;
constexpr uid_t kUid = 1234;
const std::string kClientReceiverIdentifier = "/" + std::string("");

class DatarouterMessageClientFactoryFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        auto memory_resource = score::cpp::pmr::get_default_resource();
        auto unistd_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::UnistdMock>>(memory_resource);
        auto pthread_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::MockPthread>>(memory_resource);
        auto signal_mock = score::cpp::pmr::make_unique<testing::StrictMock<score::os::SignalMock>>(memory_resource);

        unistd_mock_ = unistd_mock.get();
        pthread_mock_ = pthread_mock.get();
        signal_mock_ = signal_mock.get();
        auto message_passing_factory = std::make_unique<testing::StrictMock<MessagePassingFactoryMock>>();
        message_passing_factory_ = message_passing_factory.get();
        factory_ = std::make_unique<DatarouterMessageClientFactoryImpl>(
            config,
            std::move(message_passing_factory),
            MsgClientUtils{std::move(unistd_mock), std::move(pthread_mock), std::move(signal_mock)});
    }

    void TearDown() override {}

    void ExpectUnlinkMwsrWriterFile()
    {
        EXPECT_CALL(*unistd_mock_, unlink(StrEq(kMwsrFileName)));
    }

    void ExpectPidQuery()
    {
        EXPECT_CALL(*unistd_mock_, getpid).WillOnce(Return(kThisProcessPid));
    }

    void ExpectUidQuery()
    {
        EXPECT_CALL(*unistd_mock_, getuid).WillOnce(Return(kUid));
    }

    std::unique_ptr<score::mw::log::detail::DatarouterMessageClient> CreateClientWithFactory()
    {
        ExpectPidQuery();
        ExpectUidQuery();
        ExpectUnlinkMwsrWriterFile();
        return factory_->CreateOnce("", "");
    }

  protected:
    testing::StrictMock<score::os::UnistdMock>* unistd_mock_;
    testing::StrictMock<score::os::MockPthread>* pthread_mock_;
    testing::StrictMock<score::os::SignalMock>* signal_mock_;
    std::unique_ptr<DatarouterMessageClientImpl> client_;
    testing::StrictMock<MessagePassingFactoryMock>* message_passing_factory_;
    std::unique_ptr<DatarouterMessageClientFactoryImpl> factory_;
    const score::mw::log::detail::Configuration config{};
};

TEST_F(DatarouterMessageClientFactoryFixture, CreateOnceShouldReturnClientWithExpectedValues)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability to instantiate a client.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    auto client = CreateClientWithFactory();

    auto client_impl = dynamic_cast<DatarouterMessageClientImpl*>(client.get());

    // Using the getters check that the factory provided the expected values to the constructor.

    EXPECT_EQ(client_impl->GetReceiverIdentifier(), kClientReceiverIdentifier);
    EXPECT_EQ(client_impl->GetAppid(), LoggingIdentifier{config.GetAppId()});
    EXPECT_EQ(client_impl->GetThisProcessPid(), kThisProcessPid);
    EXPECT_EQ(client_impl->GetWriterFileName(), kMwsrFileName);
}

TEST_F(DatarouterMessageClientFactoryFixture, CallingCreateMoreThanOnceShallAbort)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of instantiating a client only once.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    std::ignore = CreateClientWithFactory();

    EXPECT_DEATH(factory_->CreateOnce("", ""), "");
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
