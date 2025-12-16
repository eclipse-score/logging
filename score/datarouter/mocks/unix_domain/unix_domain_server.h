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

#ifndef UNIX_DOMAIN_SERVER_H_
#define UNIX_DOMAIN_SERVER_H_

#include "i_session.h"
#include "unix_domain/unix_domain_common.h"

#include <functional>
#include <optional>
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
class UnixDomainServer
{
  public:
    class SessionHandle
    {
      public:
        SessionHandle(int,
                      UnixDomainServer*,
                      std::optional<std::reference_wrapper<std::string>> set_response_reference = std::nullopt)
            : last_message_{set_response_reference}
        {
        }

        void pass_message(const std::string& message) const
        {
            if (last_message_.has_value())
            {
                last_message_.value().get() = message;
            }
        }

      private:
        std::optional<std::reference_wrapper<std::string>> last_message_;
    };

    using ISession = score::logging::ISession;

    using SessionFactory = std::function<std::unique_ptr<ISession>(int, const std::string&, SessionHandle)>;

    using Session2Factory = std::function<std::unique_ptr<ISession>(const std::string&, SessionHandle)>;

    UnixDomainServer(UnixDomainSockAddr, Session2Factory = Session2Factory()) {}
    ~UnixDomainServer() {}
};

namespace dummy_namespace
{
// dummy type to avoid warnings Wsuggest-final-types and Wsuggest-final-methods
struct temp_marker final : UnixDomainServer::ISession
{
    bool tick() override
    {
        return false;
    };
    void on_command(const std::string&) override {}
    void on_closed_by_peer() override {}
};
}  // namespace dummy_namespace

}  // namespace mock

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // UNIX_DOMAIN_SERVER_H_
