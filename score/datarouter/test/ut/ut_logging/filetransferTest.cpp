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

#include "gtest/gtest.h"

namespace score
{
namespace logging
{
namespace
{

#if defined(DLT_FILE_TRANSFER_FEATURE)
TEST(FileTransferTest, FileTransferShouldNotFail)
{
    score::logging::IFileTransferPtr file_transfer = std::make_unique<score::logging::FileTransfer>("FTEA", "FMSG");
    std::string fname = "/opt/filetransfer/etc/context.3155760015.MsmStateSetterGetter.887.txt";
    EXPECT_NO_FATAL_FAILURE(file_transfer->TransferFile(fname, false));
}
#endif

}  // namespace
}  // namespace logging
}  // namespace score
