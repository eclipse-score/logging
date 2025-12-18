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

#include "score/mw/log/detail/data_router/message_passing_factory_impl.h"

#include "score/mw/com/message_passing/receiver_factory_impl.h"
#include "score/mw/com/message_passing/sender_factory_impl.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

score::cpp::pmr::unique_ptr<score::mw::com::message_passing::IReceiver> MessagePassingFactoryImpl::CreateReceiver(
    const std::string_view identifier,
    concurrency::Executor& executor,
    const score::cpp::span<const uid_t> allowed_user_ids,
    const score::mw::com::message_passing::ReceiverConfig& receiver_config,
    score::cpp::pmr::memory_resource* const monotonic_memory_resource) noexcept
{
    return score::mw::com::message_passing::ReceiverFactoryImpl::Create(
        identifier, executor, allowed_user_ids, receiver_config, monotonic_memory_resource);
}

score::cpp::pmr::unique_ptr<score::mw::com::message_passing::ISender> MessagePassingFactoryImpl::CreateSender(
    const std::string_view identifier,
    const score::cpp::stop_token& token,
    const score::mw::com::message_passing::SenderConfig& sender_config,
    score::mw::com::message_passing::LoggingCallback callback,
    score::cpp::pmr::memory_resource* const monotonic_memory_resource) noexcept
{
    return score::mw::com::message_passing::SenderFactoryImpl::Create(
        identifier, token, sender_config, std::move(callback), monotonic_memory_resource);
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
