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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/un.h>

namespace score {
namespace logging {
namespace config {

namespace {
constexpr char kEnvSocketPath[] = "DATAROUTER_SOCKET_PATH";
constexpr char kEnvSocketMode[] = "DATAROUTER_SOCKET_MODE";

#ifdef __QNXNTO__
constexpr char kDefaultSocketPath[] = "/var/run/datarouter.sock";
constexpr char kDefaultSocketMode[] = "file";
#else
constexpr char kDefaultSocketPath[] = "datarouter_socket";
constexpr char kDefaultSocketMode[] = "abstract";
#endif

}  // namespace

SocketConfiguration GetSocketConfiguration() noexcept
{
    // Read environment variables
    // NOLINTNEXTLINE(concurrency-mt-unsafe) Environment read at startup is safe
    // NOLINTNEXTLINE(score-banned-function) getenv is necessary for configuration
    const char* env_path = std::getenv(kEnvSocketPath);
    // NOLINTNEXTLINE(concurrency-mt-unsafe) Environment read at startup is safe
    // NOLINTNEXTLINE(score-banned-function) getenv is necessary for configuration
    const char* env_mode = std::getenv(kEnvSocketMode);

    // Apply path with fallback to default
    std::string path;
    if (env_path != nullptr && env_path[0] != '\0')
    {
        path = env_path;
    }
    else
    {
        path = kDefaultSocketPath;
    }

    // Apply mode with fallback to default
    std::string mode_str;
    if (env_mode != nullptr)
    {
        mode_str = env_mode;
    }
    else
    {
        mode_str = kDefaultSocketMode;
    }

    // Convert mode string to boolean (case-insensitive)
    // Any value other than "file" or "FILE" is treated as abstract
    bool is_abstract = true;
    if (mode_str == "file" || mode_str == "FILE")
    {
        is_abstract = false;
    }

    // Validate path length
    // sockaddr_un.sun_path has limited size, need room for null terminator
    // and potentially the leading null byte for abstract sockets
    constexpr std::size_t kMaxPathLength = sizeof(sockaddr_un::sun_path) - 1;
    if (path.length() >= kMaxPathLength)
    {
        std::cerr << "Warning: DATAROUTER_SOCKET_PATH too long (max " << kMaxPathLength
                  << " chars), using default: " << kDefaultSocketPath << '\n';
        path = kDefaultSocketPath;
    }

    return SocketConfiguration{path, is_abstract};
}

score::platform::internal::UnixDomainSockAddr CreateSocketAddress() noexcept
{
    auto config = GetSocketConfiguration();
    return score::platform::internal::UnixDomainSockAddr(config.path, config.is_abstract);
}

}  // namespace config
}  // namespace logging
}  // namespace score
