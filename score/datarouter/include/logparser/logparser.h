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

#include "logparser/i_logparser.h"

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

namespace internal
{

class LogParser : public ILogParser
{
  public:
    explicit LogParser(const score::mw::log::INvConfig& nv_config);
    ~LogParser() = default;

    void set_filter_factory(FilterFunctionFactory factory) override
    {
        filter_factory = factory;
    }

    void add_incoming_type(const bufsize_t map_index, const std::string& params) override;
    void AddIncomingType(const score::mw::log::detail::TypeRegistration&) override;

    void add_type_handler(const std::string& typeName, TypeHandler& handler) override;
    void add_global_handler(AnyHandler& handler) override;

    void remove_type_handler(const std::string& typeName, TypeHandler& handler) override;
    void remove_global_handler(AnyHandler& handler) override;

    bool is_type_hndl_registered(const std::string& typeName, const TypeHandler& handler) override;
    bool is_glb_hndl_registered(const AnyHandler& handler) override;

    void reset_internal_mapping() override;

    void parse(timestamp_t timestamp, const char* data, bufsize_t size) override;
    void Parse(const score::mw::log::detail::SharedMemoryRecord& record) override;

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

        explicit IndexParser(TypeInfo info) : info_{info}, handlers_() {}

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
