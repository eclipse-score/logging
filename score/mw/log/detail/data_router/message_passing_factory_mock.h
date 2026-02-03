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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_MOCK_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_MOCK_H

#include "score/mw/log/detail/data_router/message_passing_factory.h"

#include "gmock/gmock.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class MessagePassingFactoryMock : public MessagePassingFactory
{
  public:
    MOCK_METHOD((score::cpp::pmr::unique_ptr<score::mw::com::message_passing::IReceiver>),
                CreateReceiver,
                (const std::string_view,
                 concurrency::Executor&,
                 const score::cpp::span<const uid_t>,
                 const score::mw::com::message_passing::ReceiverConfig&,
                 score::cpp::pmr::memory_resource*),
                (override));

    MOCK_METHOD((score::cpp::pmr::unique_ptr<score::mw::com::message_passing::ISender>),
                CreateSender,
                (const std::string_view identifier,
                 const score::cpp::stop_token& token,
                 const score::mw::com::message_passing::SenderConfig& sender_config,
                 score::mw::com::message_passing::LoggingCallback callback,
                 score::cpp::pmr::memory_resource* memory_resource),
                (override));
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_H
