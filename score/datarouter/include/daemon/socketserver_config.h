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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_SOCKETSERVER_CONFIG_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_SOCKETSERVER_CONFIG_H

#include "daemon/dlt_log_server_config.h"
#include "score/datarouter/src/persistency/i_persistent_dictionary.h"

#include "score/result/result.h"

namespace score
{
namespace platform
{
namespace datarouter
{

score::Result<score::logging::dltserver::StaticConfig> ReadStaticDlt(const char* path);
score::logging::dltserver::PersistentConfig ReadDlt(IPersistentDictionary& pd);
void WriteDlt(const score::logging::dltserver::PersistentConfig& config, IPersistentDictionary& pd);

bool ReadDltEnabled(IPersistentDictionary& pd);
void WriteDltEnabled(bool enabled, IPersistentDictionary& pd);

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_SOCKETSERVER_CONFIG_H
