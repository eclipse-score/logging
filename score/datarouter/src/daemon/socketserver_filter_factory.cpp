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

#include "daemon/socketserver_filter_factory.h"

#include "dlt/logentry_trace.h"
#include "static_reflection_with_serialization/serialization/skip_deserialize.h"

#include "score/mw/log/detail/log_entry.h"

namespace score
{
namespace platform
{
namespace datarouter
{

namespace v = score::common::visitor;

struct LogEntryFilterable
{
    score::mw::log::detail::LoggingIdentifier app_id{""};
    score::mw::log::detail::LoggingIdentifier ctx_id{""};
    v::skip_deserialize<score::mw::log::detail::ByteVector> payload{};
    // v::skip_deserialize<std::uint64_t> timestamp_steady_nsec;
    // v::skip_deserialize<std::uint64_t> timestamp_system_nsec;
    v::skip_deserialize<int8_t> num_of_args{};
    // v::skip_deserialize<score::mw::log::detail::ByteVector> header_buffer;
    uint8_t log_level{};
};

// external linkage to suppress STRUCT_TRACEABLE unused fields warning
// Suppress "AUTOSAR C++14 A18-9-4". The rule states: An argument to std::forward shall not be subsequently used.
// Rationale: STRUCT_TRACEABLE is a macro, std::forward is from the macro definition
// Suppress "AUTOSAR C++14 M3-2-3". The rule states: A type, object or function that is used in multiple translation
// units shall be declared in one and only one file.
// Rationale: STRUCT_TRACEABLE is defined in this file with these args
// only once.
// coverity[autosar_cpp14_a18_9_4_violation]
// coverity[autosar_cpp14_m3_2_3_violation]
STRUCT_TRACEABLE(LogEntryFilterable,
                 app_id,
                 ctx_id,
                 payload,
                 //  timestamp_steady_nsec,
                 //  timestamp_system_nsec,
                 num_of_args,
                 //  header_buffer,
                 log_level)
static_assert(v::is_payload_compatible<LogEntryFilterable, score::mw::log::detail::LogEntry>(), "not compatible");

struct DataFilterable
{
    std::uint32_t serviceId;
    std::uint32_t instanceId;
    std::uint32_t attributeId;
    v::skip_deserialize<std::vector<std::uint8_t>> payload;
};

// external linkage to suppress STRUCT_TRACEABLE unused fields warning
// Suppress "AUTOSAR C++14 A18-9-4". The rule states: An argument to std::forward shall not be subsequently used.
// Rationale: STRUCT_TRACEABLE is a macro, std::forward is from the macro definition
// coverity[autosar_cpp14_a18_9_4_violation]
STRUCT_TRACEABLE(DataFilterable, serviceId, instanceId, attributeId, payload)

score::platform::internal::LogParser::FilterFunctionFactory getFilterFactory()
{
    auto factory = [](const std::string& typeName,
                      const DataFilter& filter) -> score::platform::internal::LogParser::FilterFunction {
        if (typeName == v::struct_visitable<score::mw::log::detail::LogEntry>::name() &&
            filter.filterType == v::struct_visitable<score::platform::internal::LogEntryFilter>::name())
        {
            score::platform::internal::LogEntryFilter entryFilter;
            using s = v::logging_serializer;
            if (s::deserialize(filter.filterData.data(), filter.filterData.size(), entryFilter))
            {
                return [entryFilter](const char* const data, const bufsize_t size) {
                    LogEntryFilterable entry;
                    if (!s::deserialize(data, size, entry))
                    {
                        return false;
                    }
                    const bool appIdMatch = entryFilter.app_id == score::mw::log::detail::LoggingIdentifier{""} ||
                                            entryFilter.app_id == entry.app_id;
                    const bool ctxIdMatch = entryFilter.ctx_id == score::mw::log::detail::LoggingIdentifier{""} ||
                                            // condition tested :FALSE POSITIVE.
                                            // LCOV_EXCL_START
                                            entryFilter.ctx_id == entry.ctx_id;
                    // LCOV_EXCL_STOP
                    const bool logLevelMatch = entryFilter.logLevelThreshold >= entry.log_level;
                    return appIdMatch && ctxIdMatch && logLevelMatch;
                };
            }
        }
        return {};
    };
    return factory;
}

}  // namespace datarouter
}  // namespace platform
}  // namespace score
