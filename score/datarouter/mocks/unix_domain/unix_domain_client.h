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

#ifndef UNIX_DOMAIN_CLIENT_H_
#define UNIX_DOMAIN_CLIENT_H_

#include "unix_domain/unix_domain_common.h"

#include <functional>
#include <string>

#include <gmock/gmock.h>

namespace score
{
namespace platform
{
namespace internal
{

inline namespace mock
{
class UnixDomainClient
{
  public:
    using Callback = std::function<void(void)>;
    using TickCallback = std::function<bool(void)>;
    using FdCallback = std::function<void(int)>;
    using RequestCallback = std::function<void(const std::string&)>;
    UnixDomainClient(UnixDomainSockAddr addr,
                     Callback onConnect,
                     Callback onDisconnect,
                     FdCallback onFd = FdCallback(),
                     TickCallback onTick = TickCallback(),
                     RequestCallback onRequest = RequestCallback())
    {
    }
    ~UnixDomainClient() {}

    void pass_fd(int fdout) {}

    void pass_fd(const std::string& name, int fdout) {}

    void send_response(const std::string& response) {}

    void ping() {}
};

}  // namespace mock

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // UNIX_DOMAIN_CLIENT_H_
