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

#include "score/mw/log/logging.h"

#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace log = score::mw::log;

namespace
{
constexpr char kDefaultMessage[]{"default message text for example log generating application"};

struct Config
{
    std::uint32_t threads{1U};
    std::uint32_t iterations{10U};
    std::uint32_t sleep_ms{0U};
    bool use_counter{false};
    bool use_full_output{false};
    std::uint32_t shutdown_wait_ms{0U};
};

void generate_logs(const Config& config, std::uint32_t thread_id)
{
    auto logger = log::CreateLogger("LOGG", "dlt generator context");

    for (std::uint32_t i{0U}; i < config.iterations; ++i)
    {
        if (config.use_full_output)
        {
            logger.LogFatal() << kDefaultMessage;
            logger.LogError() << kDefaultMessage;
            logger.LogWarn() << kDefaultMessage;
            logger.LogInfo() << kDefaultMessage;
            logger.LogVerbose() << kDefaultMessage;
            logger.LogDebug() << kDefaultMessage;
        }
        else if (config.use_counter)
        {
            logger.LogInfo() << "thread=" << thread_id << " iter=" << i << " " << kDefaultMessage;
        }
        else
        {
            logger.LogInfo() << kDefaultMessage;
        }

        if (config.sleep_ms > 0U)
        {
            usleep(config.sleep_ms * 1000U);
        }
    }
}

bool parse_bool(const char* str)
{
    return (std::strcmp(str, "true") == 0);
}
}  // namespace

int main(int argc, char** argv)
{
    Config config;

    for (int i{1}; i < argc - 1; i += 2)
    {
        const std::string flag{argv[i]};
        const std::string value{argv[i + 1]};

        if (flag == "-t")
        {
            config.threads = static_cast<std::uint32_t>(std::stoul(value));
        }
        else if (flag == "-i")
        {
            config.iterations = static_cast<std::uint32_t>(std::stoul(value));
        }
        else if (flag == "-s")
        {
            config.sleep_ms = static_cast<std::uint32_t>(std::stoul(value));
        }
        else if (flag == "-c")
        {
            config.use_counter = parse_bool(value.c_str());
        }
        else if (flag == "-f")
        {
            config.use_full_output = parse_bool(value.c_str());
        }
        else if (flag == "-w")
        {
            config.shutdown_wait_ms = static_cast<std::uint32_t>(std::stoul(value));
        }
    }

    if (config.threads <= 1U)
    {
        generate_logs(config, 0U);
    }
    else
    {
        std::vector<std::thread> threads;
        threads.reserve(config.threads);
        for (std::uint32_t t{0U}; t < config.threads; ++t)
        {
            threads.emplace_back(generate_logs, std::cref(config), t);
        }
        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    if (config.shutdown_wait_ms > 0U)
    {
        usleep(config.shutdown_wait_ms * 1000U);
    }

    return 0;
}
