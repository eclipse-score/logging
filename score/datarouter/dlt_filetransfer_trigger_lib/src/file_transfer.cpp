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

#include "score/datarouter/dlt_filetransfer_trigger_lib/include/file_transfer.h"

#include "score/datarouter/dlt_filetransfer_trigger_lib/filetransfer_message_trace.h"

#include <Tracing>

namespace score::logging
{

FileTransfer::FileTransfer(const std::string& appid, const std::string& ctxid)
    : IFileTransfer(), appid_(appid), ctxid_(ctxid)
{
}

void FileTransfer::TransferFile(const std::string& file_name, const bool delete_file) const
{
    FileTransferEntry entry;
    entry.appid = appid_;
    entry.ctxid = ctxid_;
    entry.file_name = file_name;
    entry.delete_file = delete_file;

    TRACE(entry);
}

}  // namespace score::logging
