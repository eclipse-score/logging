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

#ifndef SCORE_PAS_LOGGING_ERROR_ERROR_H
#define SCORE_PAS_LOGGING_ERROR_ERROR_H

#include "score/result/error.h"

namespace score
{
namespace logging
{
namespace error
{

enum class LoggingErrorCode : score::result::ErrorCode
{
    kNoFileFound = 1,
    kParseError,
    kNoChannelsFound
};

class LoggingErrorDomain final : public result::ErrorDomain
{
  public:
    std::string_view MessageFor(const result::ErrorCode& code) const noexcept override;
};

/// @brief ADL overload to fulfill design requirements from lib/result
score::result::Error MakeError(const LoggingErrorCode code, const std::string_view user_message = "") noexcept;

}  // namespace error
}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_ERROR_ERROR_H
