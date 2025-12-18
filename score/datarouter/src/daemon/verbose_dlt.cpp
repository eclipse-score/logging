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

#include "daemon/verbose_dlt.h"
#include "score/datarouter/include/daemon/log_entry_deserialization_visitor.h"

#include "static_reflection_with_serialization/serialization/for_logging.h"

namespace score
{
namespace logging
{
namespace dltserver
{

void DltVerboseHandler::handle(timestamp_t timestamp, const char* data, bufsize_t size)
{
    namespace dlt_server_logging = ::score::mw::log::detail::log_entry_deserialization;
    using dlt_duration_t = std::chrono::duration<uint32_t, std::ratio<1, 10000>>;
    uint32_t duration = std::chrono::duration_cast<dlt_duration_t>(timestamp.time_since_epoch()).count();
    dlt_server_logging::LogEntryDeserializationReflection log_entry_deserialization_reflection;
    using s = ::score::common::visitor::logging_serializer;

    s::deserialize(data, size, log_entry_deserialization_reflection);

    output_.sendVerbose(duration, log_entry_deserialization_reflection);
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
