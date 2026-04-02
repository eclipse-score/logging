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

#ifndef SCORE_DATAROUTER_INCLUDE_LOGPARSER_I_LOGPARSER_H
#define SCORE_DATAROUTER_INCLUDE_LOGPARSER_I_LOGPARSER_H

#include "dlt/dltid.h"
#include "router/data_router_types.h"

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"

namespace score
{
namespace mw
{
namespace log
{
namespace config
{
class NvMsgDescriptor;
}
class INvConfig;
}  // namespace log
}  // namespace mw
namespace platform
{

using TimestampT = score::os::HighResolutionSteadyClock::time_point;

struct TypeInfo
{
    const score::mw::log::config::NvMsgDescriptor* nv_msg_desc{nullptr};
    BufsizeT id{};
    std::string params{};
    std::string type_name{};
    DltidT ecu_id{};
    DltidT app_id{};
};

namespace internal
{

class ILogParser
{
  public:
    class TypeHandler
    {
      public:
        virtual void Handle(TimestampT timestamp, const char* data, BufsizeT size) = 0;
        virtual ~TypeHandler() = default;
    };

    class AnyHandler
    {
      public:
        virtual void Handle(const TypeInfo& type_info, TimestampT timestamp, const char* data, BufsizeT size) = 0;

        virtual ~AnyHandler() = default;
    };

    struct TypeHandlerBinding
    {
        std::string type_name;
        TypeHandler* handler;
    };

    virtual ~ILogParser() = default;

    virtual void AddIncomingType(const BufsizeT map_index, const std::string& params) = 0;
    virtual void AddIncomingType(const score::mw::log::detail::TypeRegistration&) = 0;

    virtual void Parse(TimestampT timestamp, const char* data, BufsizeT size) = 0;
    virtual void ParseSharedMemoryRecord(const score::mw::log::detail::SharedMemoryRecord& record) = 0;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_LOGPARSER_I_LOGPARSER_H
