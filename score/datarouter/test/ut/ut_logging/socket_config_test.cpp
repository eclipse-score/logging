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

#include "daemon/socket_config.h"

#include "unix_domain/unix_domain_common.h"
#include "gtest/gtest.h"
#include <cstdlib>

namespace score {
namespace logging {
namespace config {

class SocketConfigTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Save original env vars
        const char* path = std::getenv("DATAROUTER_SOCKET_PATH");
        const char* mode = std::getenv("DATAROUTER_SOCKET_MODE");
        original_path_ = path ? std::string(path) : std::string();
        original_mode_ = mode ? std::string(mode) : std::string();
        has_original_path_ = (path != nullptr);
        has_original_mode_ = (mode != nullptr);
    }

    void TearDown() override
    {
        // Restore original env vars
        if (has_original_path_)
        {
            setenv("DATAROUTER_SOCKET_PATH", original_path_.c_str(), 1);
        }
        else
        {
            unsetenv("DATAROUTER_SOCKET_PATH");
        }
        if (has_original_mode_)
        {
            setenv("DATAROUTER_SOCKET_MODE", original_mode_.c_str(), 1);
        }
        else
        {
            unsetenv("DATAROUTER_SOCKET_MODE");
        }
    }

    std::string original_path_;
    std::string original_mode_;
    bool has_original_path_ = false;
    bool has_original_mode_ = false;
};

TEST_F(SocketConfigTest, DefaultConfiguration)
{
    unsetenv("DATAROUTER_SOCKET_PATH");
    unsetenv("DATAROUTER_SOCKET_MODE");

    auto config = GetSocketConfiguration();

#ifdef __QNXNTO__
    EXPECT_EQ(config.path, "/var/run/datarouter.sock");
    EXPECT_FALSE(config.is_abstract);
#else
    EXPECT_EQ(config.path, "datarouter_socket");
    EXPECT_TRUE(config.is_abstract);
#endif
}

TEST_F(SocketConfigTest, CustomFilePath)
{
    setenv("DATAROUTER_SOCKET_PATH", "/var/run/custom.sock", 1);
    setenv("DATAROUTER_SOCKET_MODE", "file", 1);

    auto config = GetSocketConfiguration();
    EXPECT_EQ(config.path, "/var/run/custom.sock");
    EXPECT_FALSE(config.is_abstract);
}

TEST_F(SocketConfigTest, CustomAbstractPath)
{
    setenv("DATAROUTER_SOCKET_PATH", "custom_abstract", 1);
    setenv("DATAROUTER_SOCKET_MODE", "abstract", 1);

    auto config = GetSocketConfiguration();
    EXPECT_EQ(config.path, "custom_abstract");
    EXPECT_TRUE(config.is_abstract);
}

TEST_F(SocketConfigTest, InvalidModeDefaultsToAbstract)
{
    setenv("DATAROUTER_SOCKET_MODE", "invalid", 1);

    auto config = GetSocketConfiguration();
    EXPECT_TRUE(config.is_abstract);
}

TEST_F(SocketConfigTest, EmptyPathUsesDefault)
{
    setenv("DATAROUTER_SOCKET_PATH", "", 1);

    auto config = GetSocketConfiguration();
    EXPECT_FALSE(config.path.empty());
#ifdef __QNXNTO__
    EXPECT_EQ(config.path, "/var/run/datarouter.sock");
#else
    EXPECT_EQ(config.path, "datarouter_socket");
#endif
}

TEST_F(SocketConfigTest, CreateSocketAddressAbstract)
{
    setenv("DATAROUTER_SOCKET_PATH", "test_socket", 1);
    setenv("DATAROUTER_SOCKET_MODE", "abstract", 1);

    auto addr = CreateSocketAddress();
    EXPECT_TRUE(addr.IsAbstract());
    EXPECT_STREQ(addr.GetAddressString(), "test_socket");
}

TEST_F(SocketConfigTest, CreateSocketAddressFile)
{
    setenv("DATAROUTER_SOCKET_PATH", "/tmp/test.sock", 1);
    setenv("DATAROUTER_SOCKET_MODE", "file", 1);

    auto addr = CreateSocketAddress();
    EXPECT_FALSE(addr.IsAbstract());
    EXPECT_STREQ(addr.GetAddressString(), "/tmp/test.sock");
}

TEST_F(SocketConfigTest, ModeFileCaseInsensitive)
{
    setenv("DATAROUTER_SOCKET_PATH", "/tmp/test.sock", 1);
    setenv("DATAROUTER_SOCKET_MODE", "FILE", 1);

    auto config = GetSocketConfiguration();
    EXPECT_FALSE(config.is_abstract);
}

TEST_F(SocketConfigTest, PathOnlyWithoutMode)
{
    setenv("DATAROUTER_SOCKET_PATH", "/custom/socket.sock", 1);
    unsetenv("DATAROUTER_SOCKET_MODE");

    auto config = GetSocketConfiguration();
    EXPECT_EQ(config.path, "/custom/socket.sock");
#ifdef __QNXNTO__
    EXPECT_FALSE(config.is_abstract);  // QNX defaults to file
#else
    EXPECT_TRUE(config.is_abstract);   // Linux defaults to abstract
#endif
}

}  // namespace config
}  // namespace logging
}  // namespace score
