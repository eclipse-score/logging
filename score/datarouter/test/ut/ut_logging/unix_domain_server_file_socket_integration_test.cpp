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

#include "daemon/socket_config.h"
#include "score/os/mocklib/stdlib_mock.h"
#include "score/os/socket.h"
#include "score/os/stat.h"
#include "score/os/unistd.h"
#include "unix_domain/unix_domain_server.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/stat.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace
{

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

class UnixDomainServerFileSocketIntegrationTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        ASSERT_NE(score::os::Unistd::instance().getuid(), static_cast<uid_t>(0U))
            << "This integration test must run as a non-root user";

        const auto unique_suffix = std::to_string(score::os::Unistd::instance().getpid()) + "_" +
                                   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        directory_ = std::filesystem::temp_directory_path() / ("datarouter_socket_test_" + unique_suffix);
        std::error_code error;
        ASSERT_TRUE(std::filesystem::create_directory(directory_, error)) << error.message();
        ASSERT_FALSE(error) << error.message();
        std::filesystem::permissions(directory_,
                                     std::filesystem::perms::owner_all | std::filesystem::perms::group_all,
                                     std::filesystem::perm_options::replace,
                                     error);
        ASSERT_FALSE(error) << error.message();

        socket_path_ = (directory_ / "datarouter.sock").string();
        ON_CALL(stdlib_, getenv(_)).WillByDefault(Return(nullptr));
        ON_CALL(stdlib_, getenv(StrEq("DATAROUTER_SOCKET_PATH")))
            .WillByDefault(Return(const_cast<char*>(socket_path_.c_str())));
        ON_CALL(stdlib_, getenv(StrEq("DATAROUTER_SOCKET_MODE"))).WillByDefault(Return(const_cast<char*>(kFileMode)));
        EXPECT_CALL(stdlib_, getenv(StrEq("DATAROUTER_SOCKET_PATH"))).Times(::testing::AnyNumber());
        EXPECT_CALL(stdlib_, getenv(StrEq("DATAROUTER_SOCKET_MODE"))).Times(::testing::AnyNumber());
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    score::platform::internal::UnixDomainSockAddr MakeFileSocketAddress()
    {
        return score::logging::config::CreateSocketAddress(stdlib_);
    }

    bool WaitForSocketMode(score::os::StatBuffer& status) const
    {
        constexpr auto kTimeout = std::chrono::seconds(3);
        constexpr auto kPollInterval = std::chrono::milliseconds(10);
        const auto deadline = std::chrono::steady_clock::now() + kTimeout;
        do
        {
            const auto result = score::os::Stat::instance().stat(socket_path_.c_str(), status);
            if (result.has_value() && S_ISSOCK(status.st_mode) &&
                (status.st_mode & static_cast<std::uint32_t>(0777U)) == static_cast<std::uint32_t>(0660U))
            {
                return true;
            }
            std::this_thread::sleep_for(kPollInterval);
        } while (std::chrono::steady_clock::now() < deadline);
        return false;
    }

    bool WaitForClientConnection(const score::platform::internal::UnixDomainSockAddr& address) const
    {
        constexpr auto kTimeout = std::chrono::seconds(3);
        constexpr auto kPollInterval = std::chrono::milliseconds(10);
        const auto deadline = std::chrono::steady_clock::now() + kTimeout;
        do
        {
            const auto socket_result =
                score::os::Socket::instance().socket(score::os::Socket::Domain::kUnix, SOCK_STREAM, 0);
            if (socket_result.has_value())
            {
                const std::int32_t client_fd = socket_result.value();
                const auto connect_result = score::os::Socket::instance().connect(
                    client_fd,
                    static_cast<const sockaddr*>(static_cast<const void*>(&address.addr)),
                    sizeof(sockaddr_un));
                const auto close_result = score::os::Unistd::instance().close(client_fd);
                if (connect_result.has_value() && close_result.has_value())
                {
                    return true;
                }
            }
            std::this_thread::sleep_for(kPollInterval);
        } while (std::chrono::steady_clock::now() < deadline);
        return false;
    }

    static constexpr char kFileMode[] = "file";
    std::filesystem::path directory_;
    std::string socket_path_;
    score::os::StdlibMock stdlib_;
};

TEST_F(UnixDomainServerFileSocketIntegrationTest, CreatesRestrictiveSocketAndAcceptsNonRootClient)
{
    auto address = MakeFileSocketAddress();
    ASSERT_FALSE(address.IsAbstract());
    EXPECT_STREQ(address.GetAddressString(), socket_path_.c_str());

    score::platform::internal::UnixDomainServer server{address};

    score::os::StatBuffer status{};
    ASSERT_TRUE(WaitForSocketMode(status)) << "file socket was not created with mode 0660";
    EXPECT_EQ(status.st_uid, static_cast<std::uint64_t>(score::os::Unistd::instance().getuid()));
    EXPECT_EQ(status.st_gid, static_cast<std::uint64_t>(score::os::Unistd::instance().getgid()));
    EXPECT_TRUE(WaitForClientConnection(address)) << "non-root client could not connect to the file socket";
}

TEST_F(UnixDomainServerFileSocketIntegrationTest, ReplacesStaleSocketOnRestart)
{
    auto address = MakeFileSocketAddress();
    {
        score::platform::internal::UnixDomainServer server{address};
        score::os::StatBuffer status{};
        ASSERT_TRUE(WaitForSocketMode(status)) << "initial file socket was not created with mode 0660";
    }

    // The server leaves the socket pathname behind on shutdown. Starting it again
    // must unlink that stale socket before binding the same configured path.
    auto restarted_address = MakeFileSocketAddress();
    score::platform::internal::UnixDomainServer restarted_server{restarted_address};
    EXPECT_TRUE(WaitForClientConnection(restarted_address))
        << "server could not bind and accept clients after restarting on a stale "
           "socket path";

    score::os::StatBuffer status{};
    EXPECT_TRUE(WaitForSocketMode(status)) << "restarted file socket was not created with mode 0660";
}

TEST_F(UnixDomainServerFileSocketIntegrationTest, RefusesToUnlinkRegularFileAtSocketPath)
{
    // SetupServerSocket exits the process for a non-socket path. The death-test
    // child inherits this fixture mock and therefore cannot run its destructor.
    ::testing::Mock::AllowLeak(&stdlib_);

    {
        std::ofstream regular_file(socket_path_);
        ASSERT_TRUE(regular_file.is_open());
        regular_file << "keep this file";
        ASSERT_TRUE(regular_file.good());
    }

    EXPECT_EXIT(
        {
            auto address = MakeFileSocketAddress();
            score::platform::internal::UnixDomainServer server{address};
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::_Exit(EXIT_SUCCESS);
        },
        ::testing::ExitedWithCode(EXIT_FAILURE),
        "refusing to unlink");

    EXPECT_TRUE(std::filesystem::is_regular_file(socket_path_));
}

}  // namespace
