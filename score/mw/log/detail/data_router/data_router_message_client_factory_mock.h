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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_FACTORY_MOCK_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_FACTORY_MOCK_H

#include "score/mw/log/detail/data_router/data_router_message_client_factory.h"

#include "gmock/gmock.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class DatarouterMessageClientFactoryMock : public DatarouterMessageClientFactory
{
  public:
    MOCK_METHOD((std::unique_ptr<DatarouterMessageClient>),
                CreateOnce,
                (const std::string&, const std::string&),
                (override));
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_FACTORY_MOCK_H
