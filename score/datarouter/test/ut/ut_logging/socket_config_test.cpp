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

#include "score/os/mocklib/stdlib_mock.h"
#include "unix_domain/unix_domain_common.h"
#include "gtest/gtest.h"

using ::testing::Return;
using ::testing::StrEq;
using ::testing::_;

namespace score {
namespace logging {
namespace config {

class SocketConfigTest : public ::testing::Test
{
  protected:
    void SetupEnv(const char* path, const char* mode)
    {
        ON_CALL(stdlib_, getenv(StrEq("DATAROUTER_SOCKET_PATH"))).WillByDefault(Return(const_cast<char*>(path)));
        ON_CALL(stdlib_, getenv(StrEq("DATAROUTER_SOCKET_MODE"))).WillByDefault(Return(const_cast<char*>(mode)));
    }

    score::os::StdlibMock stdlib_;
};

TEST_F(SocketConfigTest, DefaultConfiguration)
{
    ON_CALL(stdlib_, getenv(_)).WillByDefault(Return(nullptr));

    auto config = GetSocketConfiguration(stdlib_);

#if defined(__QNX__)
    EXPECT_EQ(config.path, "/var/run/datarouter.sock");
    EXPECT_FALSE(config.is_abstract);
#else
    EXPECT_EQ(config.path, "datarouter_socket");
    EXPECT_TRUE(config.is_abstract);
#endif
}

TEST_F(SocketConfigTest, CustomFilePath)
{
    SetupEnv("/var/run/custom.sock", "file");

    auto config = GetSocketConfiguration(stdlib_);
    EXPECT_EQ(config.path, "/var/run/custom.sock");
    EXPECT_FALSE(config.is_abstract);
}

TEST_F(SocketConfigTest, CustomAbstractPath)
{
    SetupEnv("custom_abstract", "abstract");

    auto config = GetSocketConfiguration(stdlib_);
    EXPECT_EQ(config.path, "custom_abstract");
    EXPECT_TRUE(config.is_abstract);
}

TEST_F(SocketConfigTest, InvalidModeDefaultsToAbstract)
{
    SetupEnv(nullptr, "invalid");

    auto config = GetSocketConfiguration(stdlib_);
    EXPECT_TRUE(config.is_abstract);
}

TEST_F(SocketConfigTest, EmptyPathUsesDefault)
{
    SetupEnv("", nullptr);

    auto config = GetSocketConfiguration(stdlib_);
    EXPECT_FALSE(config.path.empty());
#if defined(__QNX__)
    EXPECT_EQ(config.path, "/var/run/datarouter.sock");
#else
    EXPECT_EQ(config.path, "datarouter_socket");
#endif
}

TEST_F(SocketConfigTest, CreateSocketAddressAbstract)
{
    SetupEnv("test_socket", "abstract");

    auto addr = CreateSocketAddress(stdlib_);
    EXPECT_TRUE(addr.IsAbstract());
    EXPECT_STREQ(addr.GetAddressString(), "test_socket");
}

TEST_F(SocketConfigTest, CreateSocketAddressFile)
{
    SetupEnv("/tmp/test.sock", "file");

    auto addr = CreateSocketAddress(stdlib_);
    EXPECT_FALSE(addr.IsAbstract());
    EXPECT_STREQ(addr.GetAddressString(), "/tmp/test.sock");
}

TEST_F(SocketConfigTest, ModeFileCaseInsensitive)
{
    SetupEnv("/tmp/test.sock", "FILE");

    auto config = GetSocketConfiguration(stdlib_);
    EXPECT_FALSE(config.is_abstract);
}

TEST_F(SocketConfigTest, PathOnlyWithoutMode)
{
    SetupEnv("/custom/socket.sock", nullptr);

    auto config = GetSocketConfiguration(stdlib_);
    EXPECT_EQ(config.path, "/custom/socket.sock");
#if defined(__QNX__)
    EXPECT_FALSE(config.is_abstract);
#else
    EXPECT_TRUE(config.is_abstract);
#endif
}

TEST_F(SocketConfigTest, PathTooLongFallsBackToDefault)
{
    std::string long_path(200, 'x');
    SetupEnv(long_path.c_str(), nullptr);

    auto config = GetSocketConfiguration(stdlib_);
#if defined(__QNX__)
    EXPECT_EQ(config.path, "/var/run/datarouter.sock");
#else
    EXPECT_EQ(config.path, "datarouter_socket");
#endif
}

}  // namespace config
}  // namespace logging
}  // namespace score
