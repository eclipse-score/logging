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

#ifndef PAS_LOGGING_ILOGPARSER_H_
#define PAS_LOGGING_ILOGPARSER_H_

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

using timestamp_t = score::os::HighResolutionSteadyClock::time_point;

struct TypeInfo
{
    const score::mw::log::config::NvMsgDescriptor* nvMsgDesc;
    bufsize_t id;
    std::string params;
    std::string typeName;
    dltid_t ecuId;
    dltid_t appId;
};

namespace internal
{

class ILogParser
{
  public:
    class TypeHandler
    {
      public:
        virtual void handle(timestamp_t timestamp, const char* data, bufsize_t size) = 0;
        virtual ~TypeHandler() = default;
    };

    class AnyHandler
    {
      public:
        virtual void handle(const TypeInfo& TypeInfo, timestamp_t timestamp, const char* data, bufsize_t size) = 0;

        virtual ~AnyHandler() = default;
    };

    virtual ~ILogParser() = default;

    // a function object to return whether the message parameter passes some encapsulted filter
    // (in order to support content-based forwarding)
    using FilterFunction = std::function<bool(const char*, bufsize_t)>;

    // a function to create such function objects based on the type of the message,
    // the type of the filter object, and the serialized payload of the filter object
    // (called on the request data provided by add_data_forwarder())
    using FilterFunctionFactory = std::function<FilterFunction(const std::string&, const DataFilter&)>;

    virtual void set_filter_factory(FilterFunctionFactory factory) = 0;

    virtual void add_incoming_type(const bufsize_t map_index, const std::string& params) = 0;
    virtual void AddIncomingType(const score::mw::log::detail::TypeRegistration&) = 0;

    virtual void add_type_handler(const std::string& typeName, TypeHandler& handler) = 0;
    virtual void add_global_handler(AnyHandler& handler) = 0;

    virtual void remove_type_handler(const std::string& typeName, TypeHandler& handler) = 0;
    virtual void remove_global_handler(AnyHandler& handler) = 0;

    virtual bool is_type_hndl_registered(const std::string& typeName, const TypeHandler& handler) = 0;
    virtual bool is_glb_hndl_registered(const AnyHandler& handler) = 0;

    virtual void reset_internal_mapping() = 0;
    virtual void parse(timestamp_t timestamp, const char* data, bufsize_t size) = 0;
    virtual void Parse(const score::mw::log::detail::SharedMemoryRecord& record) = 0;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // PAS_LOGGING_ILOGPARSER_H_
