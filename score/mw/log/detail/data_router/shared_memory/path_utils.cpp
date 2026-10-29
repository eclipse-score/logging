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

#include <cstdlib>
#include <iostream>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

namespace
{
constexpr char kDefaultSharedMemoryDir[] = "/tmp/";
constexpr char kEnvVarName[] = "SCORE_LOG_SHM_DIR";
}  // namespace

std::string GetSharedMemoryDirectory() noexcept
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe) Environment read at startup is safe
    // NOLINTNEXTLINE(score-banned-function) getenv is necessary for configuration
    const char* env_dir = std::getenv(kEnvVarName);

    if (env_dir == nullptr || env_dir[0] == '\0')
    {
        return kDefaultSharedMemoryDir;
    }

    std::string dir{env_dir};

    // Ensure trailing slash
    if (!dir.empty() && dir.back() != '/')
    {
        dir += '/';
    }

    return dir;
}

score::cpp::optional<std::string> EnsureDirectoryExists(const std::string& directory) noexcept
{
    // Remove trailing slash for stat/mkdir
    std::string dir_path = directory;
    if (!dir_path.empty() && dir_path.back() == '/')
    {
        dir_path.pop_back();
    }

    // Check if directory already exists
    struct stat st;
    // NOLINTNEXTLINE(score-banned-function) stat is necessary for directory validation
    if (stat(dir_path.c_str(), &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            return {};  // Success - directory exists
        }
        else
        {
            return std::string("Path exists but is not a directory: ") + directory;
        }
    }

    // Directory doesn't exist, try to create it
    // Mode: rwxr-xr-x (0755)
    // NOLINTNEXTLINE(score-banned-function) mkdir not available in OSAL, raw syscall justified
    /*
    Deviation from best practices:
    - Using raw POSIX mkdir() instead of OSAL abstraction
    Justification:
    - OSAL (score::os from score_baselibs) does not provide mkdir() abstraction
    - Creating directory is essential for configurable shared memory support
    - Using POSIX mkdir with restrictive permissions (0755) and proper error handling
    - This is infrastructure-level code, not application-level safety-critical path
    */
    if (mkdir(dir_path.c_str(), 0755) != 0)
    {
        const int err = errno;
        if (err == EEXIST)
        {
            // Race condition: another process created it
            // Verify it's actually a directory
            // NOLINTNEXTLINE(score-banned-function) stat justified above
            if (stat(dir_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            {
                return {};  // Success
            }
        }
        return std::string("Failed to create directory: ") + directory +
               " (errno: " + std::to_string(err) + " - " + std::strerror(err) + ")";
    }

    return {};  // Success
}

score::cpp::optional<std::string> GetAndEnsureSharedMemoryDirectory() noexcept
{
    const std::string dir = GetSharedMemoryDirectory();

    const auto ensure_result = EnsureDirectoryExists(dir);
    if (ensure_result.has_value())
    {
        std::cerr << "Shared memory directory setup failed: " << ensure_result.value() << '\n';
        return {};
    }

    return dir;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
