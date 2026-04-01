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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_VERBOSE_DLT_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_VERBOSE_DLT_H

#include "dlt/logentry_trace.h"
#include "logparser/logparser.h"
#include "score/mw/log/logger.h"

#include "daemon/dlt_log_channel.h"

#include <vector>

namespace score
{
namespace logging
{
namespace dltserver
{

class DltVerboseHandler : public LogParser::TypeHandler
{
  public:
    class IOutput
    {
      public:
        virtual void SendVerbose(
            uint32_t tmsp,
            const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry) = 0;
        virtual bool IsOutputEnabled() const noexcept = 0;

      protected:
        ~IOutput() = default;
    };
    explicit DltVerboseHandler(IOutput& output)
        : LogParser::TypeHandler(), logger_(score::mw::log::CreateLogger("vL", "Verbose logging")), output_(output)
    {
    }
    virtual void Handle(TimestampT timestamp, const char* data, BufsizeT size) override;

  private:
    score::mw::log::Logger& logger_;
    IOutput& output_;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_VERBOSE_DLT_H
