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

#ifndef PAS_LOGGING_LOGPARSER_H_
#define PAS_LOGGING_LOGPARSER_H_

#include "dlt/dltid.h"
#include "router/data_router_types.h"

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

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

class LogParser
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

    explicit LogParser(const score::mw::log::INvConfig& nv_config);

    // a function object to return whether the message parameter passes some encapsulted filter
    // (in order to support content-based forwarding)
    using FilterFunction = std::function<bool(const char*, bufsize_t)>;

    // a function to create such function objects based on the type of the message,
    // the type of the filter object, and the serialized payload of the filter object
    // (called on the request data provided by add_data_forwarder())
    using FilterFunctionFactory = std::function<FilterFunction(const std::string&, const DataFilter&)>;

    void set_filter_factory(FilterFunctionFactory factory)
    {
        filter_factory = factory;
    }

    void add_incoming_type(const bufsize_t map_index, const std::string& params);
    void AddIncomingType(const score::mw::log::detail::TypeRegistration&);

    void add_type_handler(const std::string& typeName, TypeHandler& handler);
    void add_global_handler(AnyHandler& handler);

    void remove_type_handler(const std::string& typeName, TypeHandler& handler);
    void remove_global_handler(AnyHandler& handler);

    bool is_type_hndl_registered(const std::string& typeName, const TypeHandler& handler);
    bool is_glb_hndl_registered(const AnyHandler& handler);

    void reset_internal_mapping();

    void parse(timestamp_t timestamp, const char* data, bufsize_t size);
    void Parse(const score::mw::log::detail::SharedMemoryRecord& record);

  private:
    struct HandleRequest
    {
        /*
        Deviation from Rule A9-6-1:
        - Data types used for interfacing with hardware or conforming to communication protocols shall be trivial,
        standard-layout and only contain members of types with defined sizes.
        Justification:
        - It's false positive, this class is not used to interface with hardware.
        */
        // coverity[autosar_cpp14_a9_6_1_violation : FALSE]
        TypeHandler* handler;
    };
    // typeName-keyed
    // HandleRequestMap::value_type* is not changed by unrelated insert/erase
    using HandleRequestMap = std::unordered_multimap<std::string, const HandleRequest>;

    class IndexParser
    {
      public:
        TypeInfo info_;

        explicit IndexParser(TypeInfo info) : info_{info}, handlers_{} {}

        void add_handler(const HandleRequestMap::value_type& request);
        void remove_handler(const HandleRequestMap::value_type& request);

        void parse(const timestamp_t timestamp, const char* const data, const bufsize_t size);

      private:
        struct Handler
        {
            /*
            Deviation from Rule A9-6-1:
            - Data types used for interfacing with hardware or conforming to communication protocols shall be trivial,
            standard-layout and only contain members of types with defined sizes.
            Justification:
            - It's false positive, this class is not used to interface with hardware.
            */
            // coverity[autosar_cpp14_a9_6_1_violation : FALSE]
            const HandleRequestMap::value_type* request;
            // coverity[autosar_cpp14_a9_6_1_violation : FALSE]
            TypeHandler* handler;
        };

        std::vector<Handler> handlers_;
    };

    FilterFunctionFactory filter_factory;

    HandleRequestMap handle_request_map;

    std::unordered_multimap<std::string, const bufsize_t> typename_to_index;
    std::unordered_map<bufsize_t, IndexParser> index_parser_map;

    std::vector<AnyHandler*> global_handlers;
    const score::mw::log::INvConfig& nv_config_;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // PAS_LOGGING_LOGPARSER_H_
