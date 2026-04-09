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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_PATH_UTILS_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_PATH_UTILS_H

#include <string>
#include <score/optional.hpp>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief Resolves the shared memory directory path from environment or default
/// \return The directory path with trailing slash, defaults to "/tmp/"
/// \note Reads SCORE_LOG_SHM_DIR environment variable. Result is memoized.
std::string GetSharedMemoryDirectory() noexcept;

/// \brief Ensures the shared memory directory exists, creating if necessary
/// \param directory The directory path to ensure exists
/// \return Empty optional on success, error message on failure
score::cpp::optional<std::string> EnsureDirectoryExists(const std::string& directory) noexcept;

/// \brief Complete validation and setup of shared memory directory
/// \return The validated directory path with trailing slash, or empty on failure
score::cpp::optional<std::string> GetAndEnsureSharedMemoryDirectory() noexcept;

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_PATH_UTILS_H
