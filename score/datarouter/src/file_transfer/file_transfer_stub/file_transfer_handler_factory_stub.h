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

#ifndef PAS_LOGGING_FILE_TRANSFER_HANDLER_FACTORY_STUB_H
#define PAS_LOGGING_FILE_TRANSFER_HANDLER_FACTORY_STUB_H

#include "logparser/logparser.h"
#include "score/datarouter/src/file_transfer/file_transfer_handler_factory.hpp"
#include "score/datarouter/src/file_transfer/file_transfer_stub/file_transfer_stream_handler_stub.h"

#include <gmock/gmock.h>
namespace score
{
namespace logging
{
namespace dltserver
{

// Mock implementation of the IOutput interface.
// Actually, there is no need for sendFTVerbose in testing, we forced
// to mock it since the base class is bure virtual.
class Output : public StubFileTransferStreamHandler::IOutput
{
  public:
    ~Output() = default;
    MOCK_METHOD(void,
                sendFTVerbose,
                (score::cpp::span<const std::uint8_t> data,
                 mw::log::LogLevel loglevel,
                 dltid_t appId,
                 dltid_t ctxId,
                 uint8_t nor,
                 uint32_t time_tmsp),
                (override));
};

/**
 * @brief Concrete factory that creates StubFileTransferStreamHandler instances.
 */
class StubFileTransferHandlerFactory : public FileTransferHandlerFactory<StubFileTransferHandlerFactory>
{
  public:
    explicit StubFileTransferHandlerFactory(Output& mock_output) : mock_output_(mock_output) {}

    std::unique_ptr<LogParser::TypeHandler> createConcreteHandler()
    {
        return std::make_unique<StubFileTransferStreamHandler>(mock_output_);
    }

  private:
    Output& mock_output_;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // PAS_LOGGING_FILE_TRANSFER_HANDLER_FACTORY_STUB_H
