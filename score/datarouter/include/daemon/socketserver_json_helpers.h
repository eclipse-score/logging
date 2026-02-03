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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_SOCKETSERVER_JSON_HELPERS_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_SOCKETSERVER_JSON_HELPERS_H

#include <rapidjson/document.h>

namespace score
{
namespace platform
{
namespace datarouter
{

inline rapidjson::Document CreateRjDocument()
{
    // This value is equal to rapidjson::Document::kDefaultStackCapacity, but cannot be referenced
    // directly because it is declared as private. It is needed because the constructor for
    // rapidjson::Document uses 0 as default values for pointers in its constructor, which cause
    // compiler warnings. To avoid these warnings, we must specify nullptr in its constructor, but
    // the stackCapacity is the middle parameter, so we cannot rely on its default value if we
    // specify the third argument as nullptr. As a result, we need to supply a value for
    // stackCapacity, even though the default value is okay for our use.
    constexpr size_t kRapidjsonDefaultStackCapacity = 1024U;

    return rapidjson::Document(nullptr, kRapidjsonDefaultStackCapacity, nullptr);
}

constexpr size_t kJsonReadBufferSize = 65536U;

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_SOCKETSERVER_JSON_HELPERS_H
