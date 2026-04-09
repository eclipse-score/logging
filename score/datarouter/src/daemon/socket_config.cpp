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
#include <iostream>

namespace score {
namespace logging {
namespace config {

namespace {
constexpr char kEnvSocketPath[] = "DATAROUTER_SOCKET_PATH";
constexpr char kEnvSocketMode[] = "DATAROUTER_SOCKET_MODE";

#if defined(__QNX__)
constexpr char kDefaultSocketPath[] = "/var/run/datarouter.sock";
constexpr char kDefaultSocketMode[] = "file";
#else
constexpr char kDefaultSocketPath[] = "datarouter_socket";
constexpr char kDefaultSocketMode[] = "abstract";
#endif

}  // namespace

SocketConfiguration GetSocketConfiguration(const score::os::Stdlib& stdlib) noexcept
{
    const char* env_path = stdlib.getenv(kEnvSocketPath);
    const char* env_mode = stdlib.getenv(kEnvSocketMode);

    std::string path;
    if (env_path != nullptr && env_path[0] != '\0')
    {
        path = env_path;
    }
    else
    {
        path = kDefaultSocketPath;
    }

    std::string mode_str;
    if (env_mode != nullptr)
    {
        mode_str = env_mode;
    }
    else
    {
        mode_str = kDefaultSocketMode;
    }

    bool is_abstract = true;
    if (mode_str == "file" || mode_str == "FILE")
    {
        is_abstract = false;
    }

    const std::size_t max_len = is_abstract
        ? score::platform::internal::UnixDomainSockAddr::kMaxAbstractPathLength
        : score::platform::internal::UnixDomainSockAddr::kMaxPathLength;
    if (path.length() > max_len)
    {
        std::cerr << "Warning: DATAROUTER_SOCKET_PATH too long (max " << max_len
                  << " chars), using default: " << kDefaultSocketPath << '\n';
        path = kDefaultSocketPath;
    }

    return SocketConfiguration{path, is_abstract};
}

score::platform::internal::UnixDomainSockAddr CreateSocketAddress(const score::os::Stdlib& stdlib) noexcept
{
    auto config = GetSocketConfiguration(stdlib);
    return score::platform::internal::UnixDomainSockAddr(config.path, config.is_abstract);
}

}  // namespace config
}  // namespace logging
}  // namespace score
