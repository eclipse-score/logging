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

#ifndef STUB_NONVERBOSE_DLT_H
#define STUB_NONVERBOSE_DLT_H

#include "daemon/dlt_log_channel.h"
#include "logparser/logparser.h"
#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/logger.h"
#include "score/mw/log/logging.h"

namespace score
{
namespace logging
{
namespace dltserver
{

class StubDltNonverboseHandler : public LogParser::AnyHandler
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

    explicit StubDltNonverboseHandler(IOutput&) {}

    void handle(const TypeInfo&, timestamp_t, const char*, bufsize_t) override
    {
        // Stub implementation does nothing
    }
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // STUB_NONVERBOSE_DLT_H
