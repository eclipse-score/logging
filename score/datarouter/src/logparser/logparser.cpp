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
inline std::string logger_unmemcpy(const std::string& params, T& t)
{
    std::copy_n(params.begin(), sizeof(T), score::cpp::bit_cast<char*>(&t));
    return params.substr(sizeof(T));
}

void logger_unpack_string(std::string params, std::string& str)
{
    uint32_t size = 0U;
    params = logger_unmemcpy(params, size);
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

LogParser::LogParser(const score::mw::log::INvConfig& nv_config) : nv_config_(nv_config) {}

void LogParser::IndexParser::add_handler(const LogParser::HandleRequestMap::value_type& request)
{
    handlers_.push_back(Handler{&request, request.second.handler});
}

void LogParser::IndexParser::remove_handler(const LogParser::HandleRequestMap::value_type& request)
{
    const auto finder = [&request](const auto& v) {
        return v.request == &request;
    };
    const auto it = std::find_if(handlers_.begin(), handlers_.end(), finder);
    if (it != handlers_.end())
    {
        handlers_.erase(it);
    }
}

void LogParser::IndexParser::parse(const timestamp_t timestamp, const char* const data, const bufsize_t size)
{
    for (const auto& handler : handlers_)
    {
        handler.handler->handle(timestamp, data, size);
    }
}

void LogParser::add_incoming_type(const bufsize_t map_index, const std::string& params)
{
    // params format: { dltid_t versionId{0}; dltid_t ecuId; dltid_t appId;
    //     uint32_t typenameLen; char typename[typenameLen];
    //     [optional, TBD] char payload_format_description[]; }
    if (params.size() <= 12 + sizeof(uint32_t) || params[0] != 0 || params[1] != 0 || params[2] != 0 || params[3] != 0)
    {
        // TODO: report
        return;
    }
    dltid_t ecuId{params.substr(4U, 4U)};
    dltid_t appId{params.substr(8U, 4U)};
    std::string typeName;
    logger_unpack_string(params.substr(12U), typeName);

    typename_to_index.emplace(typeName, map_index);
    IndexParser indexParser{TypeInfo{nv_config_.GetDltMsgDesc(typeName), map_index, params, typeName, ecuId, appId}};
    const auto ith_range = handle_request_map.equal_range(typeName);
    for (auto ith = ith_range.first; ith != ith_range.second; ++ith)
    {
        indexParser.add_handler(*ith);
    }
    index_parser_map.emplace(map_index, std::move(indexParser));
}

void LogParser::AddIncomingType(const score::mw::log::detail::TypeRegistration& type_registration)
{
    std::string params{type_registration.registration_data.data(),
                       score::mw::log::detail::GetDataSizeAsLength(type_registration.registration_data)};
    this->add_incoming_type(type_registration.type_id, params);
}

void LogParser::add_global_handler(AnyHandler& handler)
{
    if (is_glb_hndl_registered(handler) == false)
    {
        global_handlers.push_back(&handler);
    }
}

void LogParser::remove_global_handler(AnyHandler& handler)
{
    const auto it = std::find(global_handlers.begin(), global_handlers.end(), &handler);
    if (it != global_handlers.end())
    {
        global_handlers.erase(it);
    }
}

void LogParser::add_type_handler(const std::string& typeName, TypeHandler& handler)
{
    if (is_type_hndl_registered(typeName, handler))
    {
        return;
    }
    const auto ith = handle_request_map.emplace(typeName, HandleRequest{&handler});
    const auto iti_range = typename_to_index.equal_range(typeName);
    for (auto iti = iti_range.first; iti != iti_range.second; ++iti)
    {
        index_parser_map.at(iti->second).add_handler(*ith);
    }
}

void LogParser::remove_type_handler(const std::string& typeName, TypeHandler& handler)
{
    const auto ith_range = handle_request_map.equal_range(typeName);
    const auto finder = [&handler](const auto& v) {
        return v.second.handler == &handler;
    };
    const auto ith = std::find_if(ith_range.first, ith_range.second, finder);
    if (ith != ith_range.second)
    {
        const auto iti_range = typename_to_index.equal_range(typeName);
        for (auto iti = iti_range.first; iti != iti_range.second; ++iti)
        {
            index_parser_map.at(iti->second).remove_handler(*ith);
        }
        handle_request_map.erase(ith);
    }
}

bool LogParser::is_type_hndl_registered(const std::string& typeName, const TypeHandler& handler)
{
    bool retval = false;
    const auto ith_range = handle_request_map.equal_range(typeName);
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

bool LogParser::is_glb_hndl_registered(const AnyHandler& handler)
{
    const auto it = std::find(global_handlers.begin(), global_handlers.end(), &handler);
    bool retval = false;
    if (it != global_handlers.end())
    {
        retval = true;
    }
    return retval;
}

void LogParser::reset_internal_mapping()
{
    typename_to_index.clear();
    index_parser_map.clear();
}

void LogParser::parse(timestamp_t timestamp, const char* data, bufsize_t size)
{
    // TODO: move index storage and handling to MwsrHeader
    if (size < sizeof(bufsize_t))
    {
        return;
    }
    bufsize_t index = 0U;
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
    size -= static_cast<bufsize_t>(sizeof(index));

    const auto iParser = index_parser_map.find(index);
    if (iParser == index_parser_map.end())
    {
        // TODO: somehow report inconsistency?
        return;
    }

    auto& indexParser = iParser->second;
    indexParser.parse(timestamp, data, size);

    const auto& typeInfo = indexParser.info_;
    for (const auto handler : global_handlers)
    {
        handler->handle(typeInfo, timestamp, data, size);
    }
}

void LogParser::Parse(const score::mw::log::detail::SharedMemoryRecord& record)
{
    const auto index_parser_entry = index_parser_map.find(record.header.type_identifier);
    if (index_parser_entry == index_parser_map.end())
    {
        return;
    }

    auto& index_parser = index_parser_entry->second;

    const auto payload_length = score::mw::log::detail::GetDataSizeAsLength(record.payload);

    // We can't test the True case because:
    // - The 'payload_length' is the length of the score::cpp::span incoming variable and the max size of
    //   the 'score::cpp::span' is 'std::size_t' which is UINT_MAX (as mentioned in the cppreference).
    // - And because the 'bufsize_t' is unsigned int32.
    // So, we can't pass a bigger value than unsigned int32 to match the True case below because
    // it will overflow and reset the variable.
    if (payload_length > std::numeric_limits<bufsize_t>::max())  // LCOV_EXCL_BR_LINE
    {
        return;  // LCOV_EXCL_LINE
    }

    const auto payload_length_buf_size = static_cast<bufsize_t>(payload_length);
    const auto payload_ptr = record.payload.data();

    index_parser.parse(record.header.time_stamp, payload_ptr, payload_length_buf_size);

    const auto& type_info = index_parser.info_;
    for (const auto handler : global_handlers)
    {
        handler->handle(type_info, record.header.time_stamp, payload_ptr, payload_length_buf_size);
    }
}

}  // namespace internal
}  // namespace platform
}  // namespace score
