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

#ifndef STUB_SESSION_HANDLE_H
#define STUB_SESSION_HANDLE_H

#include <string>

namespace score
{
namespace logging
{
namespace daemon
{

// This class is a stub for session handling in the absence of dynamic configuration features.
class StubSessionHandle
{
  public:
    explicit StubSessionHandle(int /*fd*/) {}

    // Universal converting constructor to accept any real or mock SessionHandle type
    // without including their headers.
    template <typename T, typename... Rest>
    explicit StubSessionHandle(T&&, Rest&&...)
    {
    }

    void pass_message(const std::string& /*message*/) const {}
};

}  // namespace daemon
}  // namespace logging
}  // namespace score

#endif  // STUB_SESSION_HANDLE_H
