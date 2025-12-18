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

#include <gtest/gtest.h>
#include <atomic>
#include <iostream>
#include <string>

#include "daemon/socketserver.h"
#include "datarouter_app.h"
#include "options.h"

// We'll create a simple test fixture to ensure each test starts fresh.
class DatarouterAppTest : public ::testing::Test
{
  protected:
    // We'll keep an atomic_bool to pass to datarouter_app_run()
    std::atomic_bool exitRequested_{false};

    void SetUp() override
    {
        // Optionally reset options between tests so state doesn't leak
        // (You need to implement Options::resetInstance() or equivalent if possible.)
        // Options::resetInstance();

        // Reset getopt state if your parse uses getopt:
        optind = 0;
    }
};

TEST_F(DatarouterAppTest, AppInit)
{
    // Just call it. Usually logs "datarouter application Version 0.1s starting"
    // No direct assertions, but at least ensures no crash.
    EXPECT_NO_FATAL_FAILURE(score::logging::datarouter::datarouter_app_init());
}

TEST_F(DatarouterAppTest, AppRunNoAdaptiveRuntime)
{
    // Suppose your Options parser sets no_adaptive_runtime_ = true with "-n"
    int argc = 2;
    char prog[] = "testProg";
    char noAdaptiveArg[] = "-n";
    char* argv[] = {prog, noAdaptiveArg, nullptr};

    bool parseOk = score::logging::options::Options::parse(argc, argv);
    ASSERT_TRUE(parseOk);

    EXPECT_NO_FATAL_FAILURE(score::logging::datarouter::datarouter_app_run(exitRequested_));
}

TEST_F(DatarouterAppTest, AppRunPrintVersion)
{
    // Simulate parse with an argument that sets print_version_ = true
    int argc = 2;
    char prog[] = "testProg";
    char versionArg[] = "--version";
    char* argv[] = {prog, versionArg, nullptr};

    bool parseOk = score::logging::options::Options::parse(argc, argv);
    ASSERT_TRUE(parseOk);

    // We'll capture stdout to verify that "Version 0.1s" was printed
    testing::internal::CaptureStdout();
    score::logging::datarouter::datarouter_app_run(exitRequested_);
    std::string output = testing::internal::GetCapturedStdout();

    // Check if the version string is present
    EXPECT_NE(output.find("Version 0.1s"), std::string::npos)
        << "Expected to find version string in output: " << output;
}

TEST_F(DatarouterAppTest, AppRunDoNothing)
{
    // Simulate parse with an argument that sets do_nothing_ = true
    int argc = 2;
    char prog[] = "testProg";
    char doNothingArg[] = "-h";
    char* argv[] = {prog, doNothingArg, nullptr};

    // parse sets do_nothing_ = true
    bool parseOk = score::logging::options::Options::parse(argc, argv);
    ASSERT_TRUE(parseOk);

    // Now call run. Because do_nothing() is true, it should return immediately.
    EXPECT_NO_FATAL_FAILURE(score::logging::datarouter::datarouter_app_run(exitRequested_));
}

TEST_F(DatarouterAppTest, AppShutdown)
{
    // Typically logs "shutting down"
    // No direct assertion, but ensures it doesn't crash
    EXPECT_NO_FATAL_FAILURE(score::logging::datarouter::datarouter_app_shutdown());
}
