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

#ifndef PAS_LOGGING_FILE_TRANSFER_STREAM_HANDLER_STUB_H
#define PAS_LOGGING_FILE_TRANSFER_STREAM_HANDLER_STUB_H

#include "dlt/dltid.h"
#include "logparser/logparser.h"

#include "score/mw/log/logging.h"

using namespace score::platform::internal;
using namespace score::platform;

namespace score
{
namespace logging
{
namespace dltserver
{

/**
 * @brief A stub/mock implementation of a file transfer handler.
 * It inherits from TypeHandler and has a dependency on an IOutput object.
 */
class StubFileTransferStreamHandler : public LogParser::TypeHandler
{
  public:
    class IOutput
    {
      public:
        virtual void sendFTVerbose(score::cpp::span<const std::uint8_t> data,
                                   mw::log::LogLevel loglevel,
                                   dltid_t appId,
                                   dltid_t ctxId,
                                   uint8_t nor,
                                   uint32_t time_tmsp) = 0;

      protected:
        virtual ~IOutput() = default;
    };

    explicit StubFileTransferStreamHandler(IOutput& output) : _output(output)
    {
        // No expected logic to be done.
        std::ignore = _output;
    }

    ~StubFileTransferStreamHandler() = default;

    virtual void handle(timestamp_t /*timestamp*/, const char* /*buffer*/, bufsize_t /*size*/) override
    {
        score::mw::log::LogWarn() << "File transfer feature is disabled!";
    }

  private:
    // Composition: holds a reference to an IOutput implementation.
    IOutput& _output;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // PAS_LOGGING_FILE_TRANSFER_STREAM_HANDLER_STUB_H
