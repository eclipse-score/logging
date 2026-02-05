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

#ifndef SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_INCLUDE_IFILE_TRANSFER_H
#define SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_INCLUDE_IFILE_TRANSFER_H

#include <cstdint>
#include <memory>
#include <string>

namespace score::logging
{

class IFileTransfer
{
  protected:
    IFileTransfer() = default;

  public:
    virtual ~IFileTransfer() = default;

    // Delete copy constructor & copy assignment operator
    IFileTransfer(const IFileTransfer&) = delete;
    IFileTransfer& operator=(const IFileTransfer&) = delete;

    // Delete move semantics
    IFileTransfer(IFileTransfer&&) noexcept = delete;
    IFileTransfer& operator=(IFileTransfer&&) noexcept = delete;

    virtual void TransferFile(const std::string& file_name, const bool delete_file) const = 0;
};

using IFileTransferPtr = std::unique_ptr<IFileTransfer>;

}  // namespace score::logging

#endif  // SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_INCLUDE_IFILE_TRANSFER_H
