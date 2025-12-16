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

#ifndef LOGENTRY_TRACE_H_
#define LOGENTRY_TRACE_H_

#include "dlt/dltid.h"
#include "score/mw/log/detail/logging_identifier.h"
#include "static_reflection_with_serialization/serialization/for_logging.h"

namespace score
{
namespace platform
{

// The MEMCPY_SERIALIZABLE macro is part of the Tracing infrastructure and is tested independently.
// This usage is declarative only and does not contain logic specific to this module.
// Excluding from coverage to avoid inflating coverage metrics with non-functional declarations.
// LCOV_EXCL_START : See above
// MEMCPY_SERIALIZABLE won't cause data loss.
// coverity[autosar_cpp14_a4_7_1_violation]
MEMCPY_SERIALIZABLE(score::common::visitor::payload_tags::text, dltid_t)
// LCOV_EXCL_STOP

namespace internal
{
// False positive : This struct is defined only once in the project.
// coverity[autosar_cpp14_m3_2_3_violation]
struct LogEntryFilter
{
    // rule states "Member data in non-POD class types shall be private."
    // Rationale: No harm here to define the variable here as public
    // coverity[autosar_cpp14_m11_0_1_violation]
    score::mw::log::detail::LoggingIdentifier app_id{""};
    // coverity[autosar_cpp14_m11_0_1_violation]
    score::mw::log::detail::LoggingIdentifier ctx_id{""};
    // coverity[autosar_cpp14_m11_0_1_violation]
    uint8_t logLevelThreshold = 0U;
};

// The STRUCT_TRACEABLE macro is part of the Tracing infrastructure and is tested independently.
// This usage is declarative only and does not contain logic specific to this module.
// Excluding from coverage to avoid inflating coverage metrics with non-functional declarations.
// LCOV_EXCL_START : See above
// Suppress "AUTOSAR C++14 A18-9-4". The rule states: An argument to std::forward shall not be subsequently used.
// Rationale: STRUCT_TRACEABLE is a macro, std::forward is from the macro definition
// Suppress "AUTOSAR C++14 M3-2-3" rule finding. This rule states: "A type, object or function that is used in multiple
// translation units shall be declared in one and only one file.".
// coverity[autosar_cpp14_a18_9_4_violation]
// coverity[autosar_cpp14_m3_2_3_violation]
STRUCT_TRACEABLE(LogEntryFilter, app_id, ctx_id, logLevelThreshold)
// LCOV_EXCL_STOP

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // LOGENTRY_TRACE_H_
