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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_DIAGNOSTIC_JOB_HANDLER_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_DIAGNOSTIC_JOB_HANDLER_H

#include <cstdint>
#include <string>

#include "score/datarouter/include/daemon/i_dlt_log_server.h"

namespace score
{
namespace logging
{
namespace dltserver
{

class IDiagnosticJobHandler
{
  public:
    virtual const std::string execute(IDltLogServer& srv) = 0;
    virtual ~IDiagnosticJobHandler() = default;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_DIAGNOSTIC_JOB_HANDLER_H
