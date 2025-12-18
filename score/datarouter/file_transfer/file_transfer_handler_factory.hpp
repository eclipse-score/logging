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

#ifndef PAS_LOGGING_FILE_TRANSFER_HANDLER_FACTORY_H
#define PAS_LOGGING_FILE_TRANSFER_HANDLER_FACTORY_H

#include "logparser/logparser.h"
#include <memory>

namespace score
{
namespace logging
{
namespace dltserver
{

// Needed by LogParser.
using namespace score::platform::internal;

/**
 * @brief A factory for creating log handlers.
 */
template <typename DerivedFactory>
class FileTransferHandlerFactory
{
  public:
    std::unique_ptr<LogParser::TypeHandler> create()
    {
        return static_cast<DerivedFactory&>(*this).createConcreteHandler();
    }

  private:
    // prevent wrong inheritance
    FileTransferHandlerFactory() = default;
    ~FileTransferHandlerFactory() = default;
    FileTransferHandlerFactory(FileTransferHandlerFactory&) = delete;
    FileTransferHandlerFactory(FileTransferHandlerFactory&&) = delete;
    FileTransferHandlerFactory& operator=(FileTransferHandlerFactory&) = delete;
    FileTransferHandlerFactory& operator=(FileTransferHandlerFactory&&) = delete;

    /*
    Deviation from rule: A11-3-1
    - "Friend declarations shall not be used."
    Justification:
    - friend class and private constructor used here to prevent wrong inheritance
    */
    // coverity[autosar_cpp14_a11_3_1_violation]
    friend DerivedFactory;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // PAS_LOGGING_FILE_TRANSFER_HANDLER_FACTORY_H
