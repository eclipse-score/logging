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

#include "score/concurrency/thread_pool.h"
#include "score/mw/log/detail/data_router/message_passing_factory_impl.h"

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
const std::string kIdentifier{"/test_identifier"};

TEST(MessagePassingFactoryTests, CreateReceiverShouldReturnValue)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of creating message passing receiver.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    MessagePassingFactoryImpl factory{};

    score::concurrency::ThreadPool thread_pool{2};
    score::mw::com::message_passing::ReceiverConfig receiver_config{};
    auto receiver =
        factory.CreateReceiver(kIdentifier, thread_pool, {}, receiver_config, score::cpp::pmr::get_default_resource());
    EXPECT_NE(receiver, nullptr);
}

TEST(MessagePassingFactoryTests, CreateSenderShouldReturnValue)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of creating message passing sender.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    MessagePassingFactoryImpl factory{};

    score::cpp::stop_source s{};
    s.request_stop();
    const score::mw::com::message_passing::SenderConfig sender_config{};
    auto sender = factory.CreateSender(kIdentifier,
                                       s.get_token(),
                                       sender_config,
                                       &score::mw::com::message_passing::DefaultLoggingCallback,
                                       score::cpp::pmr::get_default_resource());
    EXPECT_NE(sender, nullptr);
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
