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

#include "nonverbose_dlt.h"

#include <chrono>

namespace score
{
namespace logging
{
namespace dltserver
{

DltNonverboseHandler::DltNonverboseHandler(IOutput& output)
    : LogParser::AnyHandler(), logger_(score::mw::log::CreateLogger("NvL", "Nonverbose logging")), output_(output)
{
}

void DltNonverboseHandler::Handle(const TypeInfo& type_info, TimestampT timestamp, const char* data, BufsizeT size)
{
    if (type_info.nv_msg_desc != nullptr)
    {
        using DltDurationT = std::chrono::duration<uint32_t, std::ratio<1, 10000>>;
        uint32_t tmsp = std::chrono::duration_cast<DltDurationT>(timestamp.time_since_epoch()).count();
        output_.SendNonVerbose(*type_info.nv_msg_desc, tmsp, data, size);
    }
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
