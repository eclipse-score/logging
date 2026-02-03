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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_PERSISTENTLOGGING_CONFIG_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_PERSISTENTLOGGING_CONFIG_H

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
        kOk,
        kErrorOpen,
        kErrorParse,
        kErrorContent
    };

    ReadResult read_result = ReadResult::kErrorOpen;
    std::vector<Plogfilterdesc> verbose_filters;
    std::vector<std::string> non_verbose_filters;
};

extern const std::string kDefaultPersistentLoggingJsonFilepath;

PersistentLoggingConfig ReadPersistentLoggingConfig(
    const std::string& file_path = kDefaultPersistentLoggingJsonFilepath);

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_PERSISTENTLOGGING_CONFIG_H
