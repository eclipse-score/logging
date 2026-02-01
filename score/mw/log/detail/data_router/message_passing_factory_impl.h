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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_IMPL_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_IMPL_H

#include "score/mw/log/detail/data_router/message_passing_factory.h"

#include "score/message_passing/client_factory.h"
#include "score/message_passing/server_factory.h"

#include <optional>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

using ServerFactory = score::message_passing::ServerFactory;
using ClientFactory = score::message_passing::ClientFactory;

class MessagePassingFactoryImpl : public MessagePassingFactory
{
  public:
    explicit MessagePassingFactoryImpl();

    score::cpp::pmr::unique_ptr<score::message_passing::IServer> CreateServer(
        const score::message_passing::ServiceProtocolConfig& protocol_config,
        const score::message_passing::IServerFactory::ServerConfig& server_config) noexcept override;

    score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection> CreateClient(
        const score::message_passing::ServiceProtocolConfig& protocol_config,
        const score::message_passing::IClientFactory::ClientConfig& client_config) noexcept override;

  private:
    ServerFactory server_factory_;
    ClientFactory client_factory_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_FACTORY_IMPL_H
