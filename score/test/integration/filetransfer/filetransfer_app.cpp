// *******************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

#include "score/datarouter/dlt_filetransfer_trigger_lib/include/file_transfer.h"
#include "score/mw/log/logger.h"
#include "score/mw/log/logging.h"

#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
constexpr std::size_t kNumTransfers{50U};
constexpr char kSourceFile[]{"/tmp/filetransfer_test_original.txt"};
}  // namespace

int main()
{
    auto logger = score::mw::log::CreateLogger("FTEA", "File Transfer Example App Context");
    score::logging::FileTransfer file_transfer("FTEA", "FMSG");

    logger.LogInfo() << "initialize";

    // Create original file
    {
        std::ofstream ofs(kSourceFile);
        ofs << "File transfer test content\n";
    }

    logger.LogInfo() << "DoFileTransfer";

    for (std::size_t i{0U}; i < kNumTransfers; ++i)
    {
        std::ostringstream oss;
        oss << "/tmp/filetransfer_test_" << i << ".txt";
        std::string filename = oss.str();

        // Copy original file to destination
        {
            std::ifstream src(kSourceFile, std::ios::binary);
            std::ofstream dst(filename, std::ios::binary);
            dst << src.rdbuf();
        }

        file_transfer.TransferFile(filename, false);
        logger.LogInfo() << "Transferred file: " << filename;
    }

    logger.LogInfo() << "done";

    sleep(2);

    return 0;
}
