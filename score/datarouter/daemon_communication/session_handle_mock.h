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

#ifndef SCORE_PAS_LOGGING_DAEMON_COMMUNICATION_SESION_HANDLE_MOCK_H_
#define SCORE_PAS_LOGGING_DAEMON_COMMUNICATION_SESION_HANDLE_MOCK_H_

#include "score/datarouter/daemon_communication/session_handle_interface.h"

#include "gmock/gmock.h"

namespace score::platform::internal::daemon::mock
{

class SessionHandleMock : public score::platform::internal::daemon::ISessionHandle
{
  public:
    MOCK_METHOD(bool, AcquireRequest, (), (const, override));
};

}  // namespace score::platform::internal::daemon::mock

#endif  //  SCORE_PAS_LOGGING_DAEMON_COMMUNICATION_SESION_HANDLE_MOCK_H_
