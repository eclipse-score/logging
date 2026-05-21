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

#include "score/mw/log/logger.h"
#include "score/mw/log/logging.h"

#include <unistd.h>

// With filtering configured for this app in the attached logging.json file,
// totally 22 ( 1 + 2 + 3 + 4 + 5 + 6 + 1 = 22) messages should be visible in DLT
int main()
{
    score::mw::log::Logger logger_1{score::mw::log::CreateLogger("FAT", "Fatal")};
    logger_1.LogFatal() << "Fatal message";
    logger_1.LogError() << "Error message";
    logger_1.LogWarn() << "Warn message";
    logger_1.LogInfo() << "Info message";
    logger_1.LogDebug() << "Debug message";
    logger_1.LogVerbose() << "Verbose message";

    score::mw::log::Logger logger_2{score::mw::log::CreateLogger("ERR", "Error")};
    logger_2.LogFatal() << "Fatal message";
    logger_2.LogError() << "Error message";
    logger_2.LogWarn() << "Warn message";
    logger_2.LogInfo() << "Info message";
    logger_2.LogDebug() << "Debug message";
    logger_2.LogVerbose() << "Verbose message";

    score::mw::log::Logger logger_3{score::mw::log::CreateLogger("WRN", "Warn")};
    logger_3.LogFatal() << "Fatal message";
    logger_3.LogError() << "Error message";
    logger_3.LogWarn() << "Warn message";
    logger_3.LogInfo() << "Info message";
    logger_3.LogDebug() << "Debug message";
    logger_3.LogVerbose() << "Verbose message";

    score::mw::log::Logger logger_4{score::mw::log::CreateLogger("INF", "Info")};
    logger_4.LogFatal() << "Fatal message";
    logger_4.LogError() << "Error message";
    logger_4.LogWarn() << "Warn message";
    logger_4.LogInfo() << "Info message";
    logger_4.LogDebug() << "Debug message";
    logger_4.LogVerbose() << "Verbose message";

    score::mw::log::Logger logger_5{score::mw::log::CreateLogger("DBG", "Debug")};
    logger_5.LogFatal() << "Fatal message";
    logger_5.LogError() << "Error message";
    logger_5.LogWarn() << "Warn message";
    logger_5.LogInfo() << "Info message";
    logger_5.LogDebug() << "Debug message";
    logger_5.LogVerbose() << "Verbose message";

    score::mw::log::Logger logger_6{score::mw::log::CreateLogger("VBS", "Verbose")};
    logger_6.LogFatal() << "Fatal message";
    logger_6.LogError() << "Error message";
    logger_6.LogWarn() << "Warn message";
    logger_6.LogInfo() << "Info message";
    logger_6.LogDebug() << "Debug message";
    logger_6.LogVerbose() << "Verbose message";

    score::mw::log::LogFatal() << "LogFatal";

    sleep(1);
    return 0;
}
