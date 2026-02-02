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

#include "daemon/socketserver_config.h"

#include "daemon/socketserver_json_helpers.h"
#include "score/datarouter/error/error.h"
#include "score/datarouter/include/applications/datarouter_feature_config.h"
#include "score/datarouter/include/daemon/utility.h"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <array>

#include <iostream>

namespace score
{
namespace platform
{
namespace datarouter
{

namespace
{

const std::string CONFIG_DATABASE_KEY = "dltConfig";
const std::string CONFIG_OUTPUT_ENABLED_KEY = "dltOutputEnabled";

using loglevel_t = score::logging::dltserver::loglevel_t;

inline loglevel_t ToLogLevelT(const std::string& logLevel)
{
    return static_cast<loglevel_t>(logchannel_operations::ToLogLevel(logLevel));
}

inline std::string GetStringFromLogLevelT(loglevel_t level)
{
    return logchannel_operations::GetStringFromLogLevel(static_cast<score::mw::log::LogLevel>(level));
}

}  // namespace

score::Result<score::logging::dltserver::StaticConfig> readStaticDlt(const char* path)
{
    using rapidjson::Document;
    using rapidjson::FileReadStream;
    using rapidjson::ParseResult;
    score::logging::dltserver::StaticConfig config{};

    FILE* fp = fopen(path, "r");

    if (fp == nullptr)
    {
        std::cerr << "Could not open file: " << path << std::endl;
        return score::MakeUnexpected(score::logging::error::LoggingErrorCode::kNoFileFound, "Could not open file");
    }
    std::array<char, JSON_READ_BUFFER_SIZE> readBuffer{};
    FileReadStream is(fp, readBuffer.data(), readBuffer.size());
    Document d = createRJDocument();
    ParseResult ok = d.ParseStream(is);
    fclose(fp);
    if (ok.IsError())
    {
        std::cerr << "Error parsing json file: " << path << std::endl;
        return score::MakeUnexpected(score::logging::error::LoggingErrorCode::kParseError);
    }
    if (d.IsArray())
    {
        std::cerr << "Old (incompatible) json format: " << path << std::endl;
        return score::MakeUnexpected(score::logging::error::LoggingErrorCode::kParseError);
    }
    if (!d.HasMember("channels"))
    {
        std::cerr << "No channel list: " << path << std::endl;
        return score::MakeUnexpected(score::logging::error::LoggingErrorCode::kNoChannelsFound);
    }
    const auto& channels = d["channels"];
    if (channels.ObjectEmpty())
    {
        std::cerr << "Empty channel list: " << path << std::endl;
        return score::MakeUnexpected(score::logging::error::LoggingErrorCode::kNoChannelsFound);
    }

    config.coredumpChannel = d.HasMember("coredumpChannel") ? dltid_t{d["coredumpChannel"].GetString()} : dltid_t{};
    config.defaultChannel = dltid_t{d["defaultChannel"].GetString()};

    for (auto itr = channels.MemberBegin(); itr != channels.MemberEnd(); ++itr)
    {
        const auto name = itr->name.GetString();
        const auto threshold = ToLogLevelT(itr->value["channelThreshold"].GetString());
        dltid_t ecu(itr->value["ecu"].GetString());
        const auto addr = itr->value.HasMember("address") ? itr->value["address"].GetString() : "";
        const auto port = static_cast<uint16_t>(itr->value["port"].GetUint());
        const auto dstAddress =
            itr->value.HasMember("dstAddress") ? itr->value["dstAddress"].GetString() : "239.255.42.99";
        const auto dstPort =
            static_cast<uint16_t>(itr->value.HasMember("dstPort") ? itr->value["dstPort"].GetInt() : 3490);
        const auto multicastInterface =
            itr->value.HasMember("multicastInterface") ? itr->value["multicastInterface"].GetString() : "";
        score::logging::dltserver::StaticConfig::ChannelDescription channel{
            ecu, addr, port, dstAddress, dstPort, threshold, multicastInterface};
        config.channels.emplace(name, std::move(channel));
    }

    const auto& assignments = d["channelAssignments"];
    for (auto itr1 = assignments.MemberBegin(); itr1 != assignments.MemberEnd(); ++itr1)
    {
        const dltid_t appId(itr1->name.GetString());
        const auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const dltid_t ctxId(itr2->name.GetString());
            const auto& assigned = itr2->value;
            for (unsigned int itr3 = 0; itr3 < assigned.Size(); ++itr3)
            {
                config.channelAssignments[appId][ctxId].push_back(dltid_t(assigned[itr3].GetString()));
            }
        }
    }

    if (d.HasMember("filteringEnabled"))
    {
        config.filteringEnabled = d["filteringEnabled"].GetBool();
    }
    else
    {
        config.filteringEnabled = true;
    }

    if (d.HasMember("defaultThreshold"))
    {
        config.defaultThreshold = ToLogLevelT(d["defaultThreshold"].GetString());
    }
    else if (d.HasMember("defaultThresold"))
    {
        config.defaultThreshold = ToLogLevelT(d["defaultThresold"].GetString());
    }
    else
    {
        std::cerr << "No defaultThreshold or defaultThresold found, set to kVerbose by default" << std::endl;
        config.defaultThreshold = mw::log::LogLevel::kVerbose;
    }

    const auto& thresholds = d["messageThresholds"];
    for (auto itr1 = thresholds.MemberBegin(); itr1 != thresholds.MemberEnd(); ++itr1)
    {
        const dltid_t appId(itr1->name.GetString());
        const auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const dltid_t ctxId(itr2->name.GetString());
            config.messageThresholds[appId][ctxId] = ToLogLevelT(itr2->value.GetString());
        }
    }

    if (d.HasMember("quotas"))
    {
        const std::string quotaEnforcementEnabledParamName = "quotaEnforcementEnabled";
        if (d["quotas"].HasMember(quotaEnforcementEnabledParamName.c_str()))
        {
            config.quotaEnforcementEnabled = d["quotas"][quotaEnforcementEnabledParamName.c_str()].GetBool();
        }
        else
        {
            config.quotaEnforcementEnabled = false;
        }

        const auto& throughput = d["quotas"]["throughput"];
        config.throughput.overallMbps = throughput["overallMbps"].GetDouble();
        const auto& applicationsKbps = throughput["applicationsKbps"];
        for (auto itr1 = applicationsKbps.MemberBegin(); itr1 != applicationsKbps.MemberEnd(); ++itr1)
        {
            config.throughput.applicationsKbps.emplace(dltid_t(itr1->name.GetString()), itr1->value.GetDouble());
        }
    }
    return config;
}

score::logging::dltserver::PersistentConfig readDlt(IPersistentDictionary& pd)
{
    using rapidjson::Document;
    using rapidjson::ParseResult;
    score::logging::dltserver::PersistentConfig config{};

    const std::string json = pd.GetString(CONFIG_DATABASE_KEY, "{}");

    Document d = createRJDocument();
    ParseResult ok = d.Parse(json.c_str());

    if (ok.IsError() || !d.HasMember("channels"))
    {
        return config;
    }
    const auto& channels = d["channels"];
    if (channels.ObjectEmpty())
    {
        return config;
    }

    for (auto itr = channels.MemberBegin(); itr != channels.MemberEnd(); ++itr)
    {
        const auto name = itr->name.GetString();
        const auto threshold = ToLogLevelT(itr->value["channelThreshold"].GetString());
        score::logging::dltserver::PersistentConfig::ChannelDescription channel{threshold};
        config.channels.emplace(name, channel);
    }

    const auto& assignments = d["channelAssignments"];
    for (auto itr1 = assignments.MemberBegin(); itr1 != assignments.MemberEnd(); ++itr1)
    {
        const dltid_t appId(itr1->name.GetString());
        const auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const dltid_t ctxId(itr2->name.GetString());
            const auto& assigned = itr2->value;
            for (unsigned int itr3 = 0; itr3 < assigned.Size(); ++itr3)
            {
                config.channelAssignments[appId][ctxId].push_back(dltid_t(assigned[itr3].GetString()));
            }
        }
    }

    if (d.HasMember("filteringEnabled"))
    {
        config.filteringEnabled = d["filteringEnabled"].GetBool();
    }
    else
    {
        config.filteringEnabled = true;
    }
    // TODO: fix typo
    config.defaultThreshold = ToLogLevelT(d["defaultThresold"].GetString());

    const auto& thresholds = d["messageThresholds"];
    for (auto itr1 = thresholds.MemberBegin(); itr1 != thresholds.MemberEnd(); ++itr1)
    {
        const dltid_t appId(itr1->name.GetString());
        auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const dltid_t ctxId(itr2->name.GetString());
            config.messageThresholds[appId][ctxId] = ToLogLevelT(itr2->value.GetString());
        }
    }

    return config;
}

void writeDlt(const score::logging::dltserver::PersistentConfig& config, IPersistentDictionary& pd)
{
    using rapidjson::Document;
    using rapidjson::StringBuffer;
    using rapidjson::Value;
    using rapidjson::Writer;
    Document d = createRJDocument();
    d.SetObject();
    auto& allocator = d.GetAllocator();

    // channels
    Value channels(rapidjson::kObjectType);
    for (auto& channel : config.channels)
    {
        const std::string& channelName = channel.first;
        const std::string& channelThreshold = GetStringFromLogLevelT(channel.second.channelThreshold);
        Value channelJson(rapidjson::kObjectType);
        channelJson.AddMember("channelThreshold", Value(channelThreshold.c_str(), allocator).Move(), allocator);
        channels.AddMember(Value(channelName.c_str(), allocator).Move(), channelJson.Move(), allocator);
    }
    d.AddMember("channels", channels.Move(), allocator);

    // channel assignments
    Value rAssignments(rapidjson::kObjectType);
    for (const auto& app : config.channelAssignments)
    {
        const std::string& appId = std::string(app.first);
        const auto& contexts = app.second;
        Value rContexts{rapidjson::kObjectType};
        for (const auto& ctx : contexts)
        {
            const std::string& ctxId = std::string(ctx.first);
            const auto& assigned = ctx.second;
            Value rChannels{rapidjson::kArrayType};
            for (const auto& channel : assigned)
            {
                rChannels.PushBack(Value(std::string{channel}.c_str(), allocator).Move(), allocator);
            }
            rContexts.AddMember(Value(ctxId.c_str(), allocator).Move(), rChannels.Move(), allocator);
        }
        rAssignments.AddMember(Value(appId.c_str(), allocator).Move(), rContexts.Move(), allocator);
    }
    d.AddMember("channelAssignments", rAssignments.Move(), allocator);

    d.AddMember("filteringEnabled", config.filteringEnabled, allocator);
    // TODO: fix typo
    d.AddMember(
        "defaultThresold", Value(GetStringFromLogLevelT(config.defaultThreshold).c_str(), allocator).Move(), allocator);

    Value rThresholds(rapidjson::kObjectType);
    for (const auto& app : config.messageThresholds)
    {
        const std::string& appId = std::string(app.first);
        const auto& contexts = app.second;
        Value rContexts{rapidjson::kObjectType};
        for (const auto& ctx : contexts)
        {
            const std::string& ctxId = std::string(ctx.first);
            const auto& threshold = GetStringFromLogLevelT(ctx.second);
            rContexts.AddMember(
                Value(ctxId.c_str(), allocator).Move(), Value(threshold.c_str(), allocator).Move(), allocator);
        }
        rThresholds.AddMember(Value(appId.c_str(), allocator).Move(), rContexts.Move(), allocator);
    }
    d.AddMember("messageThresholds", rThresholds.Move(), allocator);

    StringBuffer buffer(nullptr);
    Writer<StringBuffer> writer(buffer, nullptr);
    d.Accept(writer);

    const std::string json = buffer.GetString();
    pd.SetString(CONFIG_DATABASE_KEY, json);
}

bool readDltEnabled(IPersistentDictionary& pd)
{
    const bool enabled = pd.GetBool(CONFIG_OUTPUT_ENABLED_KEY, true);
    if constexpr (kPersistentConfigFeatureEnabled)
    {
        std::cout << "Loaded output enable = " << enabled << " from KVS" << std::endl;
    }
    return enabled;
}

void writeDltEnabled(bool enabled, IPersistentDictionary& pd)
{
    pd.SetBool(CONFIG_OUTPUT_ENABLED_KEY, enabled);
}

}  // namespace datarouter
}  // namespace platform
}  // namespace score
