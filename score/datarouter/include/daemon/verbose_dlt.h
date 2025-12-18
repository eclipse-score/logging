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

#ifndef VERBOSE_DLT_H_
#define VERBOSE_DLT_H_

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
        virtual void sendVerbose(
            uint32_t tmsp,
            const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry) = 0;

      protected:
        ~IOutput() = default;
    };
    explicit DltVerboseHandler(IOutput& output)
        : LogParser::TypeHandler(), logger_(score::mw::log::CreateLogger("vL", "Verbose logging")), output_(output)
    {
    }
    virtual void handle(timestamp_t timestamp, const char* data, bufsize_t size) override;

  private:
    score::mw::log::Logger& logger_;
    IOutput& output_;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // VERBOSE_DLT_H_
