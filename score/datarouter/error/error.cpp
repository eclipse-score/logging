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

#include "score/datarouter/error/error.h"

namespace score
{
namespace logging
{
namespace error
{

std::string_view LoggingErrorDomain::MessageFor(const score::result::ErrorCode& code) const noexcept
{
    /*
    Deviation from Rule A7-2-1:
    - An expression with enum underlying type shall only have values corresponding to the enumerators of the
    enumeration.
    Justification:
    - LoggingErrorCode and score::result::ErrorCode share the same underlying type.
    - In practice, code is either within the valid range of LoggingErrorCode or handled by the default case in our
    switch logic.
    - Consequently, casting is safe in this context despite the theoretical possibility of an out-of-range value.
    */
    // coverity[autosar_cpp14_a7_2_1_violation]
    const auto logging_error_code = static_cast<LoggingErrorCode>(code);

    switch (logging_error_code)
    {
        case LoggingErrorCode::kNoFileFound:
            return "No file was found";
        case LoggingErrorCode::kParseError:
            return "Error when try to parse json file";
        case LoggingErrorCode::kNoChannelsFound:
            return "No channels information found";
        default:
            return "Unknown generic error";
    }
}

namespace
{
constexpr LoggingErrorDomain logging_error_domain;
}

score::result::Error MakeError(const LoggingErrorCode code, const std::string_view user_message) noexcept
{
    return {static_cast<score::result::ErrorCode>(code), logging_error_domain, user_message};
}

}  // namespace error
}  // namespace logging
}  // namespace score
