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

#include "score/message_passing/i_client_factory.h"
#include "score/message_passing/i_server_factory.h"
#include <optional>

#ifdef __QNX__
#include "score/message_passing/qnx_dispatch/qnx_dispatch_client_factory.h"
#include "score/message_passing/qnx_dispatch/qnx_dispatch_engine.h"
#include "score/message_passing/qnx_dispatch/qnx_dispatch_server_factory.h"
#else
#include "score/message_passing/unix_domain/unix_domain_client_factory.h"
#include "score/message_passing/unix_domain/unix_domain_engine.h"
#include "score/message_passing/unix_domain/unix_domain_server_factory.h"
#endif

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/*
       Deviation from Rule A16-0-1:
       - Rule A16-0-1 (required, implementation, automated)
       The pre-processor shall only be used for unconditional and conditional file
       inclusion and include guards, and using the following directives: (1) #ifndef,
       #ifdef, (3) #if, (4) #if defined, (5) #elif, (6) #else, (7) #define, (8) #endif, (9)
       #include.
       Justification:
       - Feature Flag to enable/disable Logging Modes in Production SW.
       */
// coverity[autosar_cpp14_a16_0_1_violation] see above
#ifdef __QNX__
using ServerFactory = score::message_passing::QnxDispatchServerFactory;
using ClientFactory = score::message_passing::QnxDispatchClientFactory;
// coverity[autosar_cpp14_a16_0_1_violation] see above
#else
using ServerFactory = score::message_passing::UnixDomainServerFactory;
using ClientFactory = score::message_passing::UnixDomainClientFactory;
// coverity[autosar_cpp14_a16_0_1_violation] see above
#endif

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

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_H
