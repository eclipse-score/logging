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

#ifndef SCORE_PAS_LOGGING_DAEMON_COMMUNICATION_SESION_HANDLE_INTERFACE_H_
#define SCORE_PAS_LOGGING_DAEMON_COMMUNICATION_SESION_HANDLE_INTERFACE_H_

namespace score
{
namespace platform
{
namespace internal
{
namespace daemon
{

class ISessionHandle
{
  public:
    virtual void AcquireRequest() const = 0;
    virtual ~ISessionHandle() = default;
};

}  // namespace daemon
}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  //  SCORE_PAS_LOGGING_DAEMON_COMMUNICATION_SESION_HANDLE_INTERFACE_H_
