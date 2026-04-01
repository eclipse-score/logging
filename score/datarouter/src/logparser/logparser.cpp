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

#include "logparser/logparser.h"
#include "score/mw/log/configuration/nvconfig.h"

#include <algorithm>

#include <iostream>

#include "static_reflection_with_serialization/serialization/for_logging.h"

// TODO: remove
namespace
{

template <typename T>
inline std::string LoggerUnmemcpy(const std::string& params, T& t)
{
    std::copy_n(params.begin(), sizeof(T), score::cpp::bit_cast<char*>(&t));
    return params.substr(sizeof(T));
}

void LoggerUnpackString(std::string params, std::string& str)
{
    uint32_t size = 0U;
    params = LoggerUnmemcpy(params, size);
    // We can't test the False, the size of argument 'params' can't be negative. Suppress.
    if (size <= params.size())  // LCOV_EXCL_BR_LINE
    {
        str = params.substr(0, static_cast<size_t>(size));
    }
    else
    {
        // LCOV_EXCL_START
        str.clear();
        // TODO: remove debug print
        std::cerr << "!logger_unpack_string: wrong size" << std::endl;
        // LCOV_EXCL_STOP
    }
}

}  // namespace

namespace score
{
namespace platform
{
namespace internal
{

LogParser::LogParser(const score::mw::log::INvConfig& nv_config)
    : ILogParser(),
      handle_request_map_{},
      typename_to_index_{},
      index_parser_map_{},
      global_handlers_{},
      nv_config_(nv_config)
{
}

void LogParser::IndexParser::AddHandler(const LogParser::HandleRequestMap::value_type& request)
{
    handlers_.push_back(Handler{&request, request.second.handler});
}

void LogParser::IndexParser::Parse(const TimestampT timestamp, const char* const data, const BufsizeT size)
{
    for (const auto& handler : handlers_)
    {
        handler.handler->Handle(timestamp, data, size);
    }
}

void LogParser::AddIncomingType(const BufsizeT map_index, const std::string& params)
{
    // params format: { DltidT versionId{0}; DltidT ecuId; DltidT appId;
    //     uint32_t typenameLen; char typename[typenameLen];
    //     [optional, TBD] char payload_format_description[]; }
    if (params.size() <= 12 + sizeof(uint32_t) || params[0] != 0 || params[1] != 0 || params[2] != 0 || params[3] != 0)
    {
        // TODO: report
        return;
    }
    DltidT ecu_id{params.substr(4U, 4U)};
    DltidT app_id{params.substr(8U, 4U)};
    std::string type_name;
    LoggerUnpackString(params.substr(12U), type_name);

    IndexParser index_parser{
        TypeInfo{nv_config_.GetDltMsgDesc(type_name), map_index, params, type_name, ecu_id, app_id}};
    const auto ith_range = handle_request_map_.equal_range(type_name);
    for (auto ith = ith_range.first; ith != ith_range.second; ++ith)
    {
        index_parser.AddHandler(*ith);
    }
    // Insert into index_parser_map_ before typename_to_index_ to maintain the invariant:
    // every index visible in typename_to_index_ must have a corresponding entry in index_parser_map_.
    index_parser_map_.emplace(map_index, std::move(index_parser));
    typename_to_index_.emplace(type_name, map_index);
}

void LogParser::AddIncomingType(const score::mw::log::detail::TypeRegistration& type_registration)
{
    std::string params{type_registration.registration_data.data(),
                       score::mw::log::detail::GetDataSizeAsLength(type_registration.registration_data)};
    this->AddIncomingType(type_registration.type_id, params);
}

void LogParser::AddGlobalHandler(AnyHandler& handler)
{
    if (IsGlbHndlRegistered(handler) == false)
    {
        global_handlers_.push_back(&handler);
    }
}

void LogParser::AddTypeHandler(const std::string& type_name, TypeHandler& handler)
{
    if (IsTypeHndlRegistered(type_name, handler))
    {
        return;
    }
    const auto ith = handle_request_map_.emplace(type_name, HandleRequest{&handler});
    const auto iti_range = typename_to_index_.equal_range(type_name);
    for (auto iti = iti_range.first; iti != iti_range.second; ++iti)
    {
        const auto it = index_parser_map_.find(iti->second);
        if (it != index_parser_map_.end())
        {
            it->second.AddHandler(*ith);
        }
    }
}

bool LogParser::IsTypeHndlRegistered(const std::string& type_name, const TypeHandler& handler)
{
    bool retval = false;
    const auto ith_range = handle_request_map_.equal_range(type_name);
    const auto finder = [&handler](const auto& v) {
        return v.second.handler == &handler;
    };
    const auto ith = std::find_if(ith_range.first, ith_range.second, finder);
    if (ith != ith_range.second)
    {
        retval = true;
    }
    return retval;
}

bool LogParser::IsGlbHndlRegistered(const AnyHandler& handler)
{
    const auto it = std::find(global_handlers_.begin(), global_handlers_.end(), &handler);
    bool retval = false;
    if (it != global_handlers_.end())
    {
        retval = true;
    }
    return retval;
}

void LogParser::Parse(TimestampT timestamp, const char* data, BufsizeT size)
{
    // TODO: move index storage and handling to MwsrHeader
    if (size < sizeof(BufsizeT))
    {
        return;
    }
    BufsizeT index = 0U;
    /*
    Deviation from Rule M5-0-16:
    - A pointer operand and any pointer resulting from pointer arithmetic using that operand shall both address elements
    of the same array. Justification:
    - type case is necessary to extract index of parser from raw memory block (input data).
    - std::copy_n() uses same array for output, different array warning comes from score::cpp::bit_cast usage.
    */
    // coverity[autosar_cpp14_m5_0_16_violation] see above
    std::copy_n(data, sizeof(index), score::cpp::bit_cast<char*>(&index));

    std::advance(data, sizeof(index));
    size -= static_cast<BufsizeT>(sizeof(index));

    const auto i_parser = index_parser_map_.find(index);
    if (i_parser == index_parser_map_.end())
    {
        // TODO: somehow report inconsistency?
        return;
    }

    auto& index_parser = i_parser->second;
    index_parser.Parse(timestamp, data, size);

    const auto& type_info = index_parser.info;
    for (auto* const handler : global_handlers_)
    {
        handler->Handle(type_info, timestamp, data, size);
    }
}

void LogParser::Parse(const score::mw::log::detail::SharedMemoryRecord& record)
{
    const auto index_parser_entry = index_parser_map_.find(record.header.type_identifier);
    if (index_parser_entry == index_parser_map_.end())
    {
        return;
    }

    auto& index_parser = index_parser_entry->second;

    const auto payload_length = score::mw::log::detail::GetDataSizeAsLength(record.payload);

    // We can't test the True case because:
    // - The 'payload_length' is the length of the score::cpp::span incoming variable and the max size of
    //   the 'score::cpp::span' is 'std::size_t' which is UINT_MAX (as mentioned in the cppreference).
    // - And because the 'BufsizeT' is unsigned int32.
    // So, we can't pass a bigger value than unsigned int32 to match the True case below because
    // it will overflow and reset the variable.
    if (payload_length > std::numeric_limits<BufsizeT>::max())  // LCOV_EXCL_BR_LINE
    {
        return;  // LCOV_EXCL_LINE
    }

    const auto payload_length_buf_size = static_cast<BufsizeT>(payload_length);
    auto* const payload_ptr = record.payload.data();

    index_parser.Parse(record.header.time_stamp, payload_ptr, payload_length_buf_size);

    const auto& type_info = index_parser.info;
    for (auto* const handler : global_handlers_)
    {
        handler->Handle(type_info, record.header.time_stamp, payload_ptr, payload_length_buf_size);
    }
}

}  // namespace internal
}  // namespace platform
}  // namespace score
