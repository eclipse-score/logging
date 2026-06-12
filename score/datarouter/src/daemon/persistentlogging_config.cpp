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

const std::string kDefaultPersistentLoggingJsonFilepath = "etc/persistent-logging.json";

PersistentLoggingConfig ReadPersistentLoggingConfig(const std::string& file_path)
{
    using ReadResult = PersistentLoggingConfig::ReadResult;

    PersistentLoggingConfig config;
    auto file_deleter = [](std::FILE* f) { if (f) fclose(f); };
    using UniqueFileT = std::unique_ptr<std::FILE, decltype(file_deleter)>;
    UniqueFileT fp(std::fopen(file_path.c_str(), "r"), file_deleter);
    if (nullptr == fp)
    {
        config.read_result = ReadResult::kErrorOpen;
        return config;
    }
    std::string read_buffer(datarouter::kJsonReadBufferSize, '\0');
    rapidjson::FileReadStream is(fp.get(), read_buffer.data(), read_buffer.size());
    rapidjson::Document d = datarouter::CreateRjDocument();
    rapidjson::ParseResult ok = d.ParseStream(is);
    fp.reset();

    if (!ok)
    {
        score::mw::log::LogError() << "PersistentLoggingConfig:json parser error: "
                                 << std::string_view{rapidjson::GetParseError_En(ok.Code())};
        config.read_result = ReadResult::kErrorParse;
        return config;
    }
    if (false == d.HasMember("verbose_filters") || false == d.HasMember("nonverbose_filters"))
    {
        score::mw::log::LogError() << "PersistentLoggingConfig: json filter members not found.";
        config.read_result = ReadResult::kErrorContent;
        return config;
    }
    const auto& verbose_filters = d["verbose_filters"];
    const auto& nonverbose_filters = d["nonverbose_filters"];
    if (false == verbose_filters.IsArray() || false == nonverbose_filters.IsArray())
    {
        score::mw::log::LogError() << "PersistentLoggingConfig: json filters not array type.";
        config.read_result = ReadResult::kErrorContent;
        return config;
    }

    for (const auto& itr : verbose_filters.GetArray())
    {
        if (false == itr.HasMember("appId") || false == itr.HasMember("ctxId") || false == itr.HasMember("logLevel"))
        {
            score::mw::log::LogError() << "PersistentLoggingConfig: json appid, ctxid, ll not found.";
            config.read_result = ReadResult::kErrorContent;
            return config;
        }
        const auto& appid_value = itr.FindMember("appId")->value;
        const auto& ctxid_value = itr.FindMember("ctxId")->value;
        const auto& loglevel_value = itr.FindMember("logLevel")->value;
        if (false == appid_value.IsString() || false == ctxid_value.IsString() || false == loglevel_value.IsString())
        {
            score::mw::log::LogError() << "PersistentLoggingConfig: json appid, ctxid, ll not string type.";
            config.read_result = ReadResult::kErrorContent;
            return config;
        }
        config.verbose_filters.emplace_back(
            Plogfilterdesc{score::mw::log::detail::LoggingIdentifier{appid_value.GetString()},
                           score::mw::log::detail::LoggingIdentifier{ctxid_value.GetString()},
                           static_cast<uint8_t>(logchannel_operations::ToLogLevel(loglevel_value.GetString()))});
    }

    for (const auto& itr : nonverbose_filters.GetArray())
    {
        if (false == itr.IsString())
        {
            score::mw::log::LogError() << "PersistentLoggingConfig: non verbose filter not string type.";
            config.read_result = ReadResult::kErrorContent;
            return config;
        }
        config.non_verbose_filters.emplace_back(itr.GetString());
    }
    config.read_result = ReadResult::kOk;
    return config;
}

}  // namespace internal
}  // namespace platform
}  // namespace score
