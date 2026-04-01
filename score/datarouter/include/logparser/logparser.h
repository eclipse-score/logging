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

/// LogParser is NOT thread-safe.
///
/// Handler maps (handle_request_map_, global_handlers_) are populated once at
/// construction via constructor injection and never mutated afterward (Ticket-254408).
/// However, AddIncomingType() mutates index_parser_map_ and Parse*()/
/// ParseSharedMemoryRecord() reads it — both without synchronization.
///
/// This is currently safe because a single SourceSession owns each LogParser
/// instance, and AddIncomingType() / Parse*() are only called from the
/// SourceSession::Tick() path, which is single-threaded per session.
///
/// If the design ever evolves to allow concurrent access (e.g. parallel readers),
/// index_parser_map_ would need protection (e.g. std::shared_mutex).
class LogParser : public ILogParser
{
  public:
    explicit LogParser(const score::mw::log::INvConfig& nv_config,
                       std::vector<AnyHandler*> global_handlers = {},
                       std::vector<TypeHandlerBinding> type_handlers = {});
    ~LogParser() = default;

    void AddIncomingType(const BufsizeT map_index, const std::string& params) override;
    void AddIncomingType(const score::mw::log::detail::TypeRegistration&) override;

    void Parse(TimestampT timestamp, const char* data, BufsizeT size) override;
    void ParseSharedMemoryRecord(const score::mw::log::detail::SharedMemoryRecord& record) override;

  private:
    // typeName-keyed
    // HandleRequestMap::value_type* is not changed by unrelated insert/erase
    using HandleRequestMap = std::unordered_multimap<std::string, TypeHandler*>;

    class IndexParser
    {
      public:
        TypeInfo info;

        explicit IndexParser(TypeInfo type_info) : info{type_info}, handlers_{} {}

        void AddHandler(TypeHandler* handler);

        void Parse(const TimestampT timestamp, const char* const data, const BufsizeT size);

      private:
        std::vector<TypeHandler*> handlers_;
    };

    HandleRequestMap handle_request_map_;

    std::unordered_map<BufsizeT, IndexParser> index_parser_map_;

    std::vector<AnyHandler*> global_handlers_;
    const score::mw::log::INvConfig& nv_config_;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_LOGPARSER_LOGPARSER_H
