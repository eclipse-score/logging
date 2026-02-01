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
#include "score/message_passing/i_client_connection.h"
#include "score/message_passing/i_client_factory.h"
#include "score/message_passing/i_server.h"
#include "score/message_passing/i_server_factory.h"
#include "score/message_passing/service_protocol_config.h"
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

    virtual score::cpp::pmr::unique_ptr<score::message_passing::IServer> CreateServer(
        const score::message_passing::ServiceProtocolConfig& protocol_config,
        const score::message_passing::IServerFactory::ServerConfig& server_config) = 0;

    virtual score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> CreateClient(
        const score::message_passing::ServiceProtocolConfig& protocol_config,
        const score::message_passing::IClientFactory::ClientConfig& client_config) = 0;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_H
