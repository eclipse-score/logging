/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

#include <pthread.h>
#include <sched.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace
{

auto constexpr kMsg = "";
std::atomic_bool gKStopThread{false};
constexpr int kDefaultNumThreads = 3;

template <typename T>
void DoLog(T value)
{
    const auto number_of_iterations = 100;
    for (int i = 0; i < number_of_iterations; i++)
    {
        score::mw::log::Logger& logger{score::mw::log::CreateLogger(std::to_string(i), kMsg)};

        logger.LogFatal() << value;
        logger.LogError() << value;
        logger.LogWarn() << value;
        logger.LogInfo() << value;
        logger.LogDebug() << value;
        logger.LogVerbose() << value;

        std::ignore = logger.IsLogEnabled(score::mw::log::LogLevel::kError);
        std::ignore = logger.IsEnabled(score::mw::log::LogLevel::kError);

        score::mw::log::LogFatal() << value;
        score::mw::log::LogError() << value;
        score::mw::log::LogWarn() << value;
        score::mw::log::LogInfo() << value;
        score::mw::log::LogDebug() << value;
        score::mw::log::LogFatal() << value;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void LoggerThreadFunc(int thread_id)
{
    std::string thread_name = "LoggerThread" + std::to_string(thread_id);
    pthread_setname_np(pthread_self(), thread_name.c_str());
    while (gKStopThread == false)
    {
        DoLog(std::uint8_t{static_cast<std::uint8_t>(thread_id)});
    }
    pthread_exit(nullptr);
}

void SignalHandler(int)
{
    gKStopThread = true;
}

}  // namespace

int main(int argc, char* argv[])
{
    int num_logger_threads = kDefaultNumThreads;

    if (argc > 1)
    {
        num_logger_threads = std::atoi(argv[1]);
        if (num_logger_threads <= 0)
        {
            std::cerr << "Invalid number of threads: " << argv[1] << ", using default: " << kDefaultNumThreads
                      << std::endl;
            num_logger_threads = kDefaultNumThreads;
        }
    }

    std::cout << "Starting cross_locking_app with " << num_logger_threads << " logger threads" << std::endl;

    std::signal(SIGTERM, SignalHandler);
    score::mw::log::LogFatal() << 1;

    std::vector<std::thread> logger_threads;
    logger_threads.reserve(static_cast<std::size_t>(num_logger_threads));

    for (int i = 0; i < num_logger_threads; ++i)
    {
        logger_threads.emplace_back(LoggerThreadFunc, i);
    }

    for (std::size_t i = 0; i < logger_threads.size(); ++i)
    {
        sched_param sch_params{};
        sch_params.sched_priority = static_cast<int>(i);
        pthread_setschedparam(logger_threads[i].native_handle(), SCHED_RR, &sch_params);
    }

    for (auto& thread : logger_threads)
    {
        thread.join();
    }

    return 0;
}
