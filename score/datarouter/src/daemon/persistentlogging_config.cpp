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

#include "daemon/persistentlogging_config.h"

#include "daemon/socketserver_json_helpers.h"

#include "score/datarouter/include/daemon/utility.h"

#include "score/mw/log/logging.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <algorithm>
#include <iostream>
#include <memory>

namespace score
{
namespace platform
{
namespace internal
{

const std::string DEFAULT_PERSISTENT_LOGGING_JSON_FILEPATH = "etc/persistent-logging.json";

PersistentLoggingConfig readPersistentLoggingConfig(const std::string& filePath)
{
    using ReadResult = PersistentLoggingConfig::ReadResult;

    PersistentLoggingConfig config;
    using unique_file_t = std::unique_ptr<std::FILE, decltype(&fclose)>;
    unique_file_t fp(std::fopen(filePath.c_str(), "r"), &fclose);
    if (nullptr == fp)
    {
        config.readResult_ = ReadResult::ERROR_OPEN;
        return config;
    }
    std::string readBuffer(datarouter::JSON_READ_BUFFER_SIZE, '\0');
    rapidjson::FileReadStream is(fp.get(), readBuffer.data(), readBuffer.size());
    rapidjson::Document d = datarouter::createRJDocument();
    rapidjson::ParseResult ok = d.ParseStream(is);
    fp.reset();

    if (!ok)
    {
        score::mw::log::LogError() << "PersistentLoggingConfig:json parser error: "
                                 << std::string_view{rapidjson::GetParseError_En(ok.Code())};
        config.readResult_ = ReadResult::ERROR_PARSE;
        return config;
    }
    if (false == d.HasMember("verbose_filters") || false == d.HasMember("nonverbose_filters"))
    {
        score::mw::log::LogError() << "PersistentLoggingConfig: json filter members not found.";
        config.readResult_ = ReadResult::ERROR_CONTENT;
        return config;
    }
    const auto& verbose_filters = d["verbose_filters"];
    const auto& nonverbose_filters = d["nonverbose_filters"];
    if (false == verbose_filters.IsArray() || false == nonverbose_filters.IsArray())
    {
        score::mw::log::LogError() << "PersistentLoggingConfig: json filters not array type.";
        config.readResult_ = ReadResult::ERROR_CONTENT;
        return config;
    }

    for (const auto& itr : verbose_filters.GetArray())
    {
        if (false == itr.HasMember("appId") || false == itr.HasMember("ctxId") || false == itr.HasMember("logLevel"))
        {
            score::mw::log::LogError() << "PersistentLoggingConfig: json appid, ctxid, ll not found.";
            config.readResult_ = ReadResult::ERROR_CONTENT;
            return config;
        }
        const auto& appidValue = itr.FindMember("appId")->value;
        const auto& ctxidValue = itr.FindMember("ctxId")->value;
        const auto& loglevelValue = itr.FindMember("logLevel")->value;
        if (false == appidValue.IsString() || false == ctxidValue.IsString() || false == loglevelValue.IsString())
        {
            score::mw::log::LogError() << "PersistentLoggingConfig: json appid, ctxid, ll not string type.";
            config.readResult_ = ReadResult::ERROR_CONTENT;
            return config;
        }
        config.verboseFilters_.emplace_back(
            plogfilterdesc{score::mw::log::detail::LoggingIdentifier{appidValue.GetString()},
                           score::mw::log::detail::LoggingIdentifier{ctxidValue.GetString()},
                           static_cast<uint8_t>(logchannel_operations::ToLogLevel(loglevelValue.GetString()))});
    }

    for (const auto& itr : nonverbose_filters.GetArray())
    {
        if (false == itr.IsString())
        {
            score::mw::log::LogError() << "PersistentLoggingConfig: non verbose filter not string type.";
            config.readResult_ = ReadResult::ERROR_CONTENT;
            return config;
        }
        config.nonVerboseFilters_.emplace_back(itr.GetString());
    }
    config.readResult_ = ReadResult::OK;
    return config;
}

}  // namespace internal
}  // namespace platform
}  // namespace score
