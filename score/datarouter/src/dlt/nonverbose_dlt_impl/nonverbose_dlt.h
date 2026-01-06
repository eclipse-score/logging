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

#ifndef SCORE_PAS_LOGGING_NONVERBOSE_DLT_NONVERBOSE_DLT_H
#define SCORE_PAS_LOGGING_NONVERBOSE_DLT_NONVERBOSE_DLT_H

#include "logparser/logparser.h"
#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/logger.h"

#include "daemon/dlt_log_channel.h"

namespace score
{
namespace logging
{
namespace dltserver
{

class DltNonverboseHandler : public LogParser::AnyHandler
{
  public:
    class IOutput
    {
      public:
        virtual void SendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                                    uint32_t tmsp,
                                    const void* data,
                                    size_t size) = 0;

      protected:
        ~IOutput() = default;
    };
    explicit DltNonverboseHandler(IOutput& output);

    virtual void handle(const TypeInfo& type_info, timestamp_t timestamp, const char* data, bufsize_t size) override;

  private:
    score::mw::log::Logger& logger_;
    IOutput& output_;

    bool InitNonverboseMode();
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_NONVERBOSE_DLT_NONVERBOSE_DLT_H
