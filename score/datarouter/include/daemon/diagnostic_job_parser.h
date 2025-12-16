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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_DIAGNOSTIC_JOB_PARSER_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_DIAGNOSTIC_JOB_PARSER_H

#include <memory>
#include <optional>
#include <string>

#include "score/datarouter/include/daemon/i_diagnostic_job_parser.h"
#include "score/datarouter/include/daemon/i_dlt_log_server.h"

namespace score
{
namespace logging
{
namespace dltserver
{

score::platform::dltid_t extractId(const std::string& message, const size_t offset);

void appendId(score::platform::dltid_t name, std::string& message);

// This class is responsible for converting a raw byte string into a clean
// IDiagnosticJob object.
class DiagnosticJobParser : public IDiagnosticJobParser
{
  public:
    // Parses the raw command string.
    // Returns a unique pointer of type IDiagnosticJobHandler (Not an instance because it's abstract)
    // pointing to a derived class of the diagnostic job on success.
    // Returns nullptr if parsing fails (e.g., wrong size, unknown command).
    std::unique_ptr<IDiagnosticJobHandler> parse(const std::string& command) override;

  private:
    [[nodiscard]] std::optional<AssignmentAction> getAssignmentAction(std::uint8_t value) noexcept;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_DIAGNOSTIC_JOB_PARSER_H
