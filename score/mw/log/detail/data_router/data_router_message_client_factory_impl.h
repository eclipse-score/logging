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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_FACTORY_IMPL_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_FACTORY_IMPL_H

#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/detail/data_router/data_router_message_client_factory.h"
#include "score/mw/log/detail/data_router/data_router_message_client_utils.h"
#include "score/mw/log/detail/data_router/message_passing_factory.h"

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class DatarouterMessageClientFactoryImpl : public DatarouterMessageClientFactory
{
  public:
    explicit DatarouterMessageClientFactoryImpl(const Configuration& config,
                                                std::unique_ptr<MessagePassingFactory> message_passing_factory,
                                                MsgClientUtils msg_client_utils) noexcept;

    std::unique_ptr<DatarouterMessageClient> CreateOnce(const std::string& identifer,
                                                        const std::string& mwsr_file_name) override;

  private:
    bool created_once_;
    const Configuration& config_;
    std::unique_ptr<MessagePassingFactory> message_passing_factory_;
    MsgClientUtils msg_client_utils_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_H
