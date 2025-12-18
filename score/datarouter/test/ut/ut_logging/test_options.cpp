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
#include <string>

#include "options.h"

class OptionsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Ensure each test starts with a fresh Options instance

        // Also reset getopt's state if you plan to parse multiple times in a single process:
        optind = 0;
    }
};

TEST_F(OptionsTest, ParseNoArguments)
{
    // Simulate: ./program
    int argc = 1;
    char programName[] = "myProgram";
    char* argv[] = {programName, nullptr};

    // parse returns true on success
    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    // Check flags
    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_FALSE(opts.do_nothing());
    EXPECT_FALSE(opts.print_version());
    EXPECT_FALSE(opts.verbose());
    EXPECT_FALSE(opts.no_adaptive_runtime());
}

TEST_F(OptionsTest, ParseHelpShortOptionH)
{
    // Simulate: ./program -h
    int argc = 2;
    char programName[] = "myProgram";
    char helpOption[] = "-h";
    char* argv[] = {programName, helpOption, nullptr};

    // parse should return true
    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    // Because -h triggers usage, do_nothing_ is set (and we return early)
    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.do_nothing());
    EXPECT_FALSE(opts.print_version());
    EXPECT_FALSE(opts.verbose());
    EXPECT_FALSE(opts.no_adaptive_runtime());
}

TEST_F(OptionsTest, ParseVerboseShortOptionV)
{
    // Simulate: ./program -v
    int argc = 2;
    char programName[] = "myProgram";
    char helpOption[] = "-v";
    char* argv[] = {programName, helpOption, nullptr};

    // parse should return true
    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.verbose());
    EXPECT_TRUE(opts.do_nothing());
}

TEST_F(OptionsTest, ParseVerboseLongOptionV)
{
    // Simulate: ./program --verbose
    // Because of struct long_options, that's equivalent to '-v'
    int argc = 2;
    char programName[] = "myProgram";
    char verboseOption[] = "--verbose";
    char* argv[] = {programName, verboseOption, nullptr};

    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.verbose());
    EXPECT_TRUE(opts.do_nothing());
}

TEST_F(OptionsTest, ParseVerboseLongOptionHelp)
{
    // Simulate: ./program --help
    int argc = 2;
    char programName[] = "myProgram";
    char verboseOption[] = "--help";
    char* argv[] = {programName, verboseOption, nullptr};

    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.do_nothing());
}

TEST_F(OptionsTest, ParseVerboseLongOptionNoAdaptiveRuntime)
{
    // Simulate: ./program --no_adaptive_runtime
    int argc = 2;
    char programName[] = "myProgram";
    char verboseOption[] = "--no_adaptive_runtime";
    char* argv[] = {programName, verboseOption, nullptr};

    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.no_adaptive_runtime());
}

TEST_F(OptionsTest, ParseNoAdaptiveRuntimeShortOptionN)
{
    // Simulate: ./program -n
    int argc = 2;
    char programName[] = "myProgram";
    char noAdaptiveOption[] = "-n";
    char* argv[] = {programName, noAdaptiveOption, nullptr};

    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.no_adaptive_runtime());
    EXPECT_TRUE(opts.do_nothing());
}

TEST_F(OptionsTest, ParseNoAdaptiveRuntimeShortOptionV)
{
    // Simulate: ./program -V
    int argc = 2;
    char programName[] = "myProgram";
    char noAdaptiveOption[] = "-V";
    char* argv[] = {programName, noAdaptiveOption, nullptr};

    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.print_version());
}

TEST_F(OptionsTest, ParseVersionLongOption)
{
    // Simulate: ./program --version
    // Because of struct long_options, that's equivalent to '-V'
    int argc = 2;
    char programName[] = "myProgram";
    char versionOption[] = "--version";
    char* argv[] = {programName, versionOption, nullptr};

    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    // 'print_version_' is set, parse returns early
    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.print_version());
    EXPECT_TRUE(opts.no_adaptive_runtime());
}

TEST_F(OptionsTest, ParseUnknownOptionSimiColon)
{
    // Simulate: ./program --unknown
    int argc = 2;
    char programName[] = "myProgram";
    char unknownOption[] = ":";
    char* argv[] = {programName, unknownOption, nullptr};

    // parse should return false for unknown option
    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_TRUE(result);

    score::logging::options::Options& opts = score::logging::options::Options::get();
    EXPECT_TRUE(opts.print_version());
    EXPECT_TRUE(opts.no_adaptive_runtime());
    EXPECT_TRUE(opts.do_nothing());
}

TEST_F(OptionsTest, ParseUnknownOption)
{
    // Simulate: ./program --unknown
    int argc = 2;
    char programName[] = "myProgram";
    char unknownOption[] = "--unknown";
    char* argv[] = {programName, unknownOption, nullptr};

    // parse should return false for unknown option
    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_FALSE(result);
}

TEST_F(OptionsTest, ParseMissingArg)
{
    // The options has no short 'd' that requires an argument, but let's demonstrate anyway:
    // If 'd' were in `":d:"`, it might require an argument. Then parse would return false if absent.

    // We'll show a hypothetical:

    int argc = 2;
    char programName[] = "myProgram";
    char missingArgOption[] = "-d";
    char* argv[] = {programName, missingArgOption, nullptr};

    bool result = score::logging::options::Options::parse(argc, argv);

    EXPECT_FALSE(result);
}

TEST_F(OptionsTest, ParseUnknownOptionDoubleQuestionDashDash)
{
    // Simulate: ./program ??--
    int argc = 2;
    char programName[] = "myProgram";
    char unknownOption[] = "--\?\?--";
    char* argv[] = {programName, unknownOption, nullptr};

    // parse should return false for this unknown option
    bool result = score::logging::options::Options::parse(argc, argv);
    EXPECT_FALSE(result);
}
