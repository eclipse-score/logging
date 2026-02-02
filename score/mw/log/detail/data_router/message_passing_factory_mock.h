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
    MOCK_METHOD((score::cpp::pmr::unique_ptr<score::message_passing::IServer>),
                CreateServer,
                (const score::message_passing::ServiceProtocolConfig& protocol_config,
                 const score::message_passing::IServerFactory::ServerConfig& server_config),
                (override));

    MOCK_METHOD((score::cpp::pmr::unique_ptr<score::message_passing::IClientConnection>),
                CreateClient,
                (const score::message_passing::ServiceProtocolConfig& protocol_config,
                 const score::message_passing::IClientFactory::ClientConfig& client_config),
                (override));
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_H
