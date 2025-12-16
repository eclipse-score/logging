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

#ifndef PERSISTENTLOGGING_CONFIG_H_
#define PERSISTENTLOGGING_CONFIG_H_

#include "dlt/plogfilterdesc.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace score
{
namespace platform
{
namespace internal
{

struct PersistentLoggingConfig
{
    enum class ReadResult
    {
        OK,
        ERROR_OPEN,
        ERROR_PARSE,
        ERROR_CONTENT
    };

    ReadResult readResult_ = ReadResult::ERROR_OPEN;
    std::vector<plogfilterdesc> verboseFilters_;
    std::vector<std::string> nonVerboseFilters_;
};

extern const std::string DEFAULT_PERSISTENT_LOGGING_JSON_FILEPATH;

PersistentLoggingConfig readPersistentLoggingConfig(
    const std::string& filePath = DEFAULT_PERSISTENT_LOGGING_JSON_FILEPATH);

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // PERSISTENTLOGGING_CONFIG_H_
