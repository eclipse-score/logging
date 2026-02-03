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

#ifndef SCORE_DATAROUTER_SRC_CONFIGURATION_DYNAMIC_CONFIG_I_SESSION_H
#define SCORE_DATAROUTER_SRC_CONFIGURATION_DYNAMIC_CONFIG_I_SESSION_H

#include <string>

namespace score
{
namespace logging
{

class ISession
{
  public:
    virtual bool Tick() = 0;
    virtual void OnCommand(const std::string& /*command*/) {}
    virtual void OnClosedByPeer() {}
    virtual ~ISession() = default;
};

}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_SRC_CONFIGURATION_DYNAMIC_CONFIG_I_SESSION_H
