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

#ifndef PAS_LOGGING_FILE_TRANSFER_STREAM_HANDLER_FACTORY_H
#define PAS_LOGGING_FILE_TRANSFER_STREAM_HANDLER_FACTORY_H

#include "logparser/logparser.h"
#include "score/datarouter/src/file_transfer/file_transfer_handler_factory.hpp"
#include "score/datarouter/src/file_transfer/file_transfer_impl/filetransfer_stream.h"

namespace score
{
namespace logging
{
namespace dltserver
{

// implementation of the IOutput interface.
class Output : public FileTransferStreamHandler::IOutput
{
  public:
    ~Output() = default;
    void SendFtVerbose(score::cpp::span<const std::uint8_t> data,
                       mw::log::LogLevel loglevel,
                       DltidT app_id,
                       DltidT ctx_id,
                       uint8_t nor,
                       uint32_t time_tmsp) override
    {
        // The implementation for this method is not agreed currently.
        // To be discussed: Ticket-211317
        std::ignore = data;
        std::ignore = loglevel;
        std::ignore = app_id;
        std::ignore = ctx_id;
        std::ignore = nor;
        std::ignore = time_tmsp;
    }
    bool IsOutputEnabled() const noexcept override
    {
        return true;
    }
};

/**
 * @brief Concrete factory that creates FileTransferStreamHandler instances.
 */
class FileTransferStreamHandlerFactory : public FileTransferHandlerFactory<FileTransferStreamHandlerFactory>
{
  public:
    explicit FileTransferStreamHandlerFactory(Output& output) : output_(output) {}

    std::unique_ptr<LogParser::TypeHandler> CreateConcreteHandler()
    {
        return std::make_unique<FileTransferStreamHandler>(output_);
    }

  private:
    Output& output_;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // PAS_LOGGING_FILE_TRANSFER_STREAM_HANDLER_FACTORY_H
