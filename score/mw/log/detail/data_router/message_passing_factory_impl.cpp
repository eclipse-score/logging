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

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

MessagePassingFactoryImpl::MessagePassingFactoryImpl()
    : MessagePassingFactory(), server_factory_{}, client_factory_{server_factory_.GetEngine()}
{
}

score::cpp::pmr::unique_ptr<score::message_passing::IServer> MessagePassingFactoryImpl::CreateServer(
    const score::message_passing::ServiceProtocolConfig& protocol_config,
    const score::message_passing::IServerFactory::ServerConfig& server_config) noexcept
{
    return server_factory_.Create(protocol_config, server_config);
}

score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> MessagePassingFactoryImpl::CreateClient(
    const score::message_passing::ServiceProtocolConfig& protocol_config,
    const score::message_passing::IClientFactory::ClientConfig& client_config) noexcept
{
    return client_factory_.Create(protocol_config, client_config);
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
