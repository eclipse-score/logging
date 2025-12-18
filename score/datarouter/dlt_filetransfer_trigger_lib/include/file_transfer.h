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

#ifndef BMW_LIB_LOGGING_FILE_TRANSFER_H
#define BMW_LIB_LOGGING_FILE_TRANSFER_H

#include "ifile_transfer.h"

namespace score::logging
{

class FileTransfer final : public IFileTransfer
{
  public:
    FileTransfer(const std::string& appid, const std::string& ctxid);
    void TransferFile(const std::string& file_name, const bool delete_file) const override;

  private:
    std::string appid_;
    std::string ctxid_;
};

}  // namespace score::logging

#endif  // BMW_LIB_LOGGING_IFILE_TRANSFER_H
