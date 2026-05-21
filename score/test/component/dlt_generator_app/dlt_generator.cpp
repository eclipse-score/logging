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
#include "score/mw/log/logger.h"
#include "score/mw/log/logging.h"

#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace log = score::mw::log;

namespace {

const std::string kDefaultMessage = "default message text for example log generating application";

void DltLogJob(log::Logger& logger,
               uint32_t iterations,
               uint32_t sleep_ms,
               bool counter,
               bool use_full_output,
               std::optional<int32_t> thread_id) {
    for (uint32_t i = 0; i < iterations; ++i) {
        std::string prefix;
        if (thread_id.has_value()) {
            prefix = "t" + std::to_string(thread_id.value()) + ":";
        }
        if (counter) {
            prefix += std::to_string(i + 1) + ": ";
        }

        if (use_full_output) {
            logger.LogFatal() << prefix << kDefaultMessage;
            logger.LogError() << prefix << kDefaultMessage;
            logger.LogWarn() << prefix << kDefaultMessage;
        }
        logger.LogInfo() << prefix << kDefaultMessage;
        logger.LogVerbose() << prefix << kDefaultMessage;
        logger.LogDebug() << prefix << kDefaultMessage;

        if (sleep_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    uint32_t threads_num = 1;
    uint32_t iterations = 1;
    uint32_t sleep_ms = 100;
    bool counter = false;
    bool use_full_output = true;
    uint32_t sleep_before_shutdown_ms = 500;

    int opt;
    while ((opt = getopt(argc, argv, "t:i:s:c:f:w:")) != -1) {
        switch (opt) {
            case 't':
                threads_num = static_cast<uint32_t>(std::atoi(optarg));
                break;
            case 'i':
                iterations = static_cast<uint32_t>(std::atoi(optarg));
                break;
            case 's':
                sleep_ms = static_cast<uint32_t>(std::atoi(optarg));
                break;
            case 'c':
                counter = (std::string(optarg) == "true");
                break;
            case 'f':
                use_full_output = (std::string(optarg) == "true");
                break;
            case 'w':
                sleep_before_shutdown_ms = static_cast<uint32_t>(std::atoi(optarg));
                break;
            default:
                std::cerr << "Usage: " << argv[0]
                          << " [-t threads] [-i iterations] [-s sleep_ms]"
                          << " [-c true|false] [-f true|false] [-w shutdown_wait_ms]"
                          << std::endl;
                return 1;
        }
    }

    log::Logger& logger = log::CreateLogger("LOGG", "dlt generator context");
    logger.LogInfo() << "Welcome warm-up message to initialize logging context.";

    std::vector<std::thread> threads;
    for (uint32_t tid = 0; tid < threads_num; ++tid) {
        std::optional<int32_t> pass_tid =
            (threads_num == 1) ? std::nullopt : std::optional<int32_t>(static_cast<int32_t>(tid));
        threads.emplace_back(DltLogJob, std::ref(logger), iterations, sleep_ms,
                             counter, use_full_output, pass_tid);
    }

    for (auto& t : threads) {
        t.join();
    }

    logger.LogInfo() << "DLT generator finished.";

    if (sleep_before_shutdown_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_before_shutdown_ms));
    }

    return 0;
}
