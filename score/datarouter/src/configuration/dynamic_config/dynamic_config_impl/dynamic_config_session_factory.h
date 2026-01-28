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

#ifndef DYNAMIC_CONFIG_SESSION_FACTORY_H
#define DYNAMIC_CONFIG_SESSION_FACTORY_H

#include "config_session_factory.hpp"
#include "dynamic_config_session.h"

namespace score
{
namespace logging
{
namespace daemon
{

class DynamicConfigSessionFactory : public ConfigSessionFactory<DynamicConfigSessionFactory>
{
  public:
    template <typename Handler>
    std::unique_ptr<ISession> CreateConcreteSession(score::platform::internal::UnixDomainServer::SessionHandle handle,
                                                    Handler&& handler)
    {
        return std::make_unique<ConfigSession>(std::move(handle), std::forward<Handler>(handler));
    }
};

}  // namespace daemon
}  // namespace logging
}  // namespace score

#endif  // DYNAMIC_CONFIG_SESSION_FACTORY_H
