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

#ifndef DIAGNOSTIC_JOB_PARSER_MOCK_H_
#define DIAGNOSTIC_JOB_PARSER_MOCK_H_

#include <gmock/gmock.h>

#include "score/datarouter/include/daemon/i_diagnostic_job_parser.h"

namespace score
{
namespace logging
{
namespace dltserver
{

namespace mock
{
class DiagnosticJobParserMock : public IDiagnosticJobParser
{
  public:
    MOCK_METHOD(std::unique_ptr<IDiagnosticJobHandler>, parse, (const std::string&), (override));
};

}  // namespace mock

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // DIAGNOSTIC_JOB_PARSER_MOCK_H_
