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

#include "score/mw/log/detail/data_router/shared_memory/path_utils.h"
#include "gtest/gtest.h"
#include <cstdlib>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class PathUtilsTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Save original env var
        const char* orig = std::getenv("SCORE_LOG_SHM_DIR");
        original_env_ = orig ? std::string(orig) : std::string();
        has_original_ = (orig != nullptr);
    }

    void TearDown() override
    {
        // Restore original env var
        if (has_original_)
        {
            setenv("SCORE_LOG_SHM_DIR", original_env_.c_str(), 1);
        }
        else
        {
            unsetenv("SCORE_LOG_SHM_DIR");
        }
    }

    std::string original_env_;
    bool has_original_ = false;
};

TEST_F(PathUtilsTest, DefaultsToTmpWhenEnvNotSet)
{
    unsetenv("SCORE_LOG_SHM_DIR");
    EXPECT_EQ("/tmp/", GetSharedMemoryDirectory());
}

TEST_F(PathUtilsTest, ReadsFromEnvironmentVariable)
{
    setenv("SCORE_LOG_SHM_DIR", "/dev/shm", 1);
    EXPECT_EQ("/dev/shm/", GetSharedMemoryDirectory());
}

TEST_F(PathUtilsTest, AddsTrailingSlash)
{
    setenv("SCORE_LOG_SHM_DIR", "/custom/path", 1);
    EXPECT_EQ("/custom/path/", GetSharedMemoryDirectory());
}

TEST_F(PathUtilsTest, PreservesTrailingSlash)
{
    setenv("SCORE_LOG_SHM_DIR", "/custom/path/", 1);
    EXPECT_EQ("/custom/path/", GetSharedMemoryDirectory());
}

TEST_F(PathUtilsTest, EmptyEnvUsesDefault)
{
    setenv("SCORE_LOG_SHM_DIR", "", 1);
    EXPECT_EQ("/tmp/", GetSharedMemoryDirectory());
}

TEST_F(PathUtilsTest, EnsureDirectoryExistsForTmp)
{
    auto result = EnsureDirectoryExists("/tmp/");
    EXPECT_FALSE(result.has_value()) << "Should succeed for /tmp";
}

TEST_F(PathUtilsTest, EnsureDirectoryExistsForTmpWithoutSlash)
{
    auto result = EnsureDirectoryExists("/tmp");
    EXPECT_FALSE(result.has_value()) << "Should succeed for /tmp";
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
