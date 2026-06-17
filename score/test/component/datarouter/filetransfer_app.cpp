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
#include "score/mw/log/logger.h"
#include "score/mw/log/logging.h"

#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>

namespace mw_log = score::mw::log;

namespace {

const std::string kTestFileContent =
    "This is test content for file transfer integration test.\n"
    "Line 2 of test data.\n"
    "Line 3 of test data.\n";

void CreateTestFile(const std::string& path) {
    std::ofstream out(path);
    out << kTestFileContent;
}

}  // namespace

int main() {
    mw_log::Logger& logger = mw_log::CreateLogger("FTEA", "File Transfer Example App");

    logger.LogInfo() << "File Transfer Example Application" << "initialize";

    // Create a test file to transfer
    const std::string original_file = "/tmp/filetransfer_test_file.txt";
    CreateTestFile(original_file);
    logger.LogInfo() << "Created test file:" << original_file;

    // Initialize file transfer
    score::logging::FileTransfer file_transfer("FTEA", "FMSG");

    logger.LogInfo() << "File Transfer Example Application" << "DoFileTransfer";

    // Transfer multiple copies of the file
    constexpr int kNumTransfers = 50;
    for (int i = 0; i < kNumTransfers; ++i) {
        std::stringstream filename;
        filename << "/tmp/filetransfer_test_" << i << ".txt";

        {
            std::ifstream src(original_file, std::ios::binary);
            std::ofstream dst(filename.str(), std::ios::binary | std::ios::trunc);
            if (!src || !dst || !(dst << src.rdbuf())) {
                logger.LogError() << "Failed to copy file:" << filename.str();
                continue;
            }
        }

        file_transfer.TransferFile(filename.str(), false);
        logger.LogInfo() << "Transferred file:" << filename.str();
    }

    logger.LogInfo() << "File Transfer Example Application" << "done";

    // Wait for logs to be processed
    sleep(2);

    return 0;
}
