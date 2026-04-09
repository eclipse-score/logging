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

#ifndef SCORE_DATAROUTER_DAEMON_SOCKET_CONFIG_H
#define SCORE_DATAROUTER_DAEMON_SOCKET_CONFIG_H

#include <string>

// Forward declaration to avoid including the full header
namespace score {
namespace platform {
namespace internal {
class UnixDomainSockAddr;
}
}  // namespace platform
}  // namespace score

namespace score {
namespace logging {
namespace config {

/// \brief Socket configuration resolved from environment variables or defaults
struct SocketConfiguration {
    std::string path;
    bool is_abstract;
};

/// \brief Get socket configuration from environment variables or platform defaults
/// \return Socket configuration with path and abstract mode flag
/// \note Reads DATAROUTER_SOCKET_PATH and DATAROUTER_SOCKET_MODE environment variables
/// \note Platform defaults: Linux uses abstract namespace, QNX uses file-based
SocketConfiguration GetSocketConfiguration() noexcept;

/// \brief Create configured UnixDomainSockAddr from environment or defaults
/// \return Configured socket address ready for binding or connecting
/// \note This is a convenience function that combines GetSocketConfiguration()
///       with UnixDomainSockAddr construction
score::platform::internal::UnixDomainSockAddr CreateSocketAddress() noexcept;

}  // namespace config
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_DAEMON_SOCKET_CONFIG_H
