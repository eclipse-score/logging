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

// With filtering configured for this app in the attached logging.json file,
// totally 22 ( 1 + 2 + 3 + 4 + 5 + 6 + 1 = 22) messages should be visible in DLT
int main()
{
    score::mw::log::LogFatal("FAT") << "Fatal message";
    score::mw::log::LogError("FAT") << "Error message";
    score::mw::log::LogWarn("FAT") << "Warning message";
    score::mw::log::LogInfo("FAT") << "Info message";
    score::mw::log::LogDebug("FAT") << "Debug message";
    score::mw::log::LogVerbose("FAT") << "Verbose message";

    score::mw::log::LogFatal("ERR") << "Fatal message";
    score::mw::log::LogError("ERR") << "Error message";
    score::mw::log::LogWarn("ERR") << "Warning message";
    score::mw::log::LogInfo("ERR") << "Info message";
    score::mw::log::LogDebug("ERR") << "Debug message";
    score::mw::log::LogVerbose("ERR") << "Verbose message";

    score::mw::log::LogFatal("WRN") << "Fatal message";
    score::mw::log::LogError("WRN") << "Error message";
    score::mw::log::LogWarn("WRN") << "Warning message";
    score::mw::log::LogInfo("WRN") << "Info message";
    score::mw::log::LogDebug("WRN") << "Debug message";
    score::mw::log::LogVerbose("WRN") << "Verbose message";

    score::mw::log::LogFatal("INF") << "Fatal message";
    score::mw::log::LogError("INF") << "Error message";
    score::mw::log::LogWarn("INF") << "Warning message";
    score::mw::log::LogInfo("INF") << "Info message";
    score::mw::log::LogDebug("INF") << "Debug message";
    score::mw::log::LogVerbose("INF") << "Verbose message";

    score::mw::log::LogFatal("DBG") << "Fatal message";
    score::mw::log::LogError("DBG") << "Error message";
    score::mw::log::LogWarn("DBG") << "Warning message";
    score::mw::log::LogInfo("DBG") << "Info message";
    score::mw::log::LogDebug("DBG") << "Debug message";
    score::mw::log::LogVerbose("DBG") << "Verbose message";

    score::mw::log::LogFatal("VBS") << "Fatal message";
    score::mw::log::LogError("VBS") << "Error message";
    score::mw::log::LogWarn("VBS") << "Warning message";
    score::mw::log::LogInfo("VBS") << "Info message";
    score::mw::log::LogDebug("VBS") << "Debug message";
    score::mw::log::LogVerbose("VBS") << "Verbose message";

    score::mw::log::LogFatal() << "LogFatal";

    sleep(1);
    return 0;
}
