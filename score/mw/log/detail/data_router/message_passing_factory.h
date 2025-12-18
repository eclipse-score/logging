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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_H

#include "score/span.hpp"
#include "score/concurrency/executor.h"
#include "score/mw/com/message_passing/i_receiver.h"
#include "score/mw/com/message_passing/i_sender.h"
#include "score/mw/com/message_passing/receiver_config.h"
#include "score/mw/com/message_passing/sender_config.h"
#include <string_view>

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class MessagePassingFactory
{
  public:
    MessagePassingFactory() = default;
    virtual ~MessagePassingFactory();
    MessagePassingFactory(const MessagePassingFactory&) = delete;
    MessagePassingFactory(MessagePassingFactory&&) = delete;
    MessagePassingFactory& operator=(const MessagePassingFactory&) = delete;
    MessagePassingFactory& operator=(MessagePassingFactory&&) = delete;

    virtual score::cpp::pmr::unique_ptr<score::mw::com::message_passing::IReceiver> CreateReceiver(
        const std::string_view identifier,
        concurrency::Executor& executor,
        const score::cpp::span<const uid_t> allowed_user_ids,
        const score::mw::com::message_passing::ReceiverConfig& receiver_config,
        score::cpp::pmr::memory_resource* memory_resource) = 0;

    virtual score::cpp::pmr::unique_ptr<score::mw::com::message_passing::ISender> CreateSender(
        const std::string_view identifier,
        const score::cpp::stop_token& token,
        const score::mw::com::message_passing::SenderConfig& sender_config,
        score::mw::com::message_passing::LoggingCallback callback,
        score::cpp::pmr::memory_resource* memory_resource) = 0;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_H
