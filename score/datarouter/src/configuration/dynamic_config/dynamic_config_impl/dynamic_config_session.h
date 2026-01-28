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

#ifndef DYNAMIC_CONFIG_SESSION_H
#define DYNAMIC_CONFIG_SESSION_H

#include "unix_domain/unix_domain_server.h"
#include <functional>
namespace score
{
namespace logging
{
namespace daemon
{

class ConfigSession final : public score::platform::internal::UnixDomainServer::ISession
{
  public:
    template <typename Handler>
    ConfigSession(score::platform::internal::UnixDomainServer::SessionHandle handle, Handler&& handler)
        : handle_(std::move(handle)), handler_(std::forward<Handler>(handler))
    {
    }

    ~ConfigSession() override = default;

  private:
    // Not called from production code.
    // LCOV_EXCL_START
    bool tick() override
    {
        return false;
    }
    // LCOV_EXCL_STOP
    void on_command(const std::string& command) override final
    {
        auto response = handler_(command);
        handle_.pass_message(response);
    }

    score::platform::internal::UnixDomainServer::SessionHandle handle_;
    std::function<std::string(const std::string&)> handler_;
};

}  // namespace daemon
}  // namespace logging
}  // namespace score

#endif  // DYNAMIC_CONFIG_SESSION_H
