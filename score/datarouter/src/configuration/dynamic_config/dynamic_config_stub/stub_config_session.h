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

#ifndef STUB_CONFIG_SESSION_H
#define STUB_CONFIG_SESSION_H

#include "i_session.h"
#include "stub_session_handle.h"
#include <functional>

namespace score
{
namespace logging
{
namespace daemon
{

class StubConfigSession : public score::logging::ISession
{
  public:
    template <typename Handle, typename Handler>
    StubConfigSession(Handle&&, Handler&&)
    {
    }
    bool tick() override
    {
        return true;
    }
    void on_command(const std::string&) override {}
    void on_closed_by_peer() override {}
};

}  // namespace daemon
}  // namespace logging
}  // namespace score

#endif  // STUB_CONFIG_SESSION_H
