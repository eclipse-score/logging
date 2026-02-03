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

#ifndef SCORE_DATAROUTER_INCLUDE_LOGPARSER_LOGPARSER_H
#define SCORE_DATAROUTER_INCLUDE_LOGPARSER_LOGPARSER_H

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

    void SetFilterFactory(FilterFunctionFactory factory) override
    {
        filter_factory_ = factory;
    }

    void AddIncomingType(const BufsizeT map_index, const std::string& params) override;
    void AddIncomingType(const score::mw::log::detail::TypeRegistration&) override;

    void AddTypeHandler(const std::string& type_name, TypeHandler& handler) override;
    void AddGlobalHandler(AnyHandler& handler) override;

    void RemoveTypeHandler(const std::string& type_name, TypeHandler& handler) override;
    void RemoveGlobalHandler(AnyHandler& handler) override;

    bool IsTypeHndlRegistered(const std::string& type_name, const TypeHandler& handler) override;
    bool IsGlbHndlRegistered(const AnyHandler& handler) override;

    void ResetInternalMapping() override;

    void Parse(TimestampT timestamp, const char* data, BufsizeT size) override;
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
        TypeInfo info;

        explicit IndexParser(TypeInfo type_info) : info{type_info}, handlers_{} {}

        void AddHandler(const HandleRequestMap::value_type& request);
        void RemoveHandler(const HandleRequestMap::value_type& request);

        void Parse(const TimestampT timestamp, const char* const data, const BufsizeT size);

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

    FilterFunctionFactory filter_factory_;

    HandleRequestMap handle_request_map_;

    std::unordered_multimap<std::string, const BufsizeT> typename_to_index_;
    std::unordered_map<BufsizeT, IndexParser> index_parser_map_;

    std::vector<AnyHandler*> global_handlers_;
    const score::mw::log::INvConfig& nv_config_;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_LOGPARSER_LOGPARSER_H
