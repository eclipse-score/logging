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

const std::string kConfigDatabaseKey = "dltConfig";
const std::string kConfigOutputEnabledKey = "dltOutputEnabled";

using LoglevelT = score::logging::dltserver::LoglevelT;

inline LoglevelT ToLogLevelT(const std::string& log_level)
{
    return static_cast<LoglevelT>(logchannel_operations::ToLogLevel(log_level));
}

inline std::string GetStringFromLogLevelT(LoglevelT level)
{
    return logchannel_operations::GetStringFromLogLevel(static_cast<score::mw::log::LogLevel>(level));
}

}  // namespace

score::Result<score::logging::dltserver::StaticConfig> ReadStaticDlt(const char* path)
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
    std::array<char, kJsonReadBufferSize> read_buffer{};
    FileReadStream is(fp, read_buffer.data(), read_buffer.size());
    Document d = CreateRjDocument();
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

    config.coredump_channel = d.HasMember("coredumpChannel") ? DltidT{d["coredumpChannel"].GetString()} : DltidT{};
    config.default_channel = DltidT{d["defaultChannel"].GetString()};

    for (auto itr = channels.MemberBegin(); itr != channels.MemberEnd(); ++itr)
    {
        const auto* const name = itr->name.GetString();
        const auto threshold = ToLogLevelT(itr->value["channelThreshold"].GetString());
        DltidT ecu(itr->value["ecu"].GetString());
        const auto* const addr = itr->value.HasMember("address") ? itr->value["address"].GetString() : "";
        const auto port = static_cast<uint16_t>(itr->value["port"].GetUint());
        const auto* const dst_address =
            itr->value.HasMember("dstAddress") ? itr->value["dstAddress"].GetString() : "239.255.42.99";
        const auto dst_port =
            static_cast<uint16_t>(itr->value.HasMember("dstPort") ? itr->value["dstPort"].GetInt() : 3490);
        const auto* const multicast_interface =
            itr->value.HasMember("multicastInterface") ? itr->value["multicastInterface"].GetString() : "";
        score::logging::dltserver::StaticConfig::ChannelDescription channel{
            ecu, addr, port, dst_address, dst_port, threshold, multicast_interface};
        config.channels.emplace(name, std::move(channel));
    }

    const auto& assignments = d["channelAssignments"];
    for (auto itr1 = assignments.MemberBegin(); itr1 != assignments.MemberEnd(); ++itr1)
    {
        const DltidT app_id(itr1->name.GetString());
        const auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const DltidT ctx_id(itr2->name.GetString());
            const auto& assigned = itr2->value;
            for (unsigned int itr3 = 0; itr3 < assigned.Size(); ++itr3)
            {
                config.channel_assignments[app_id][ctx_id].push_back(DltidT(assigned[itr3].GetString()));
            }
        }
    }

    if (d.HasMember("filteringEnabled"))
    {
        config.filtering_enabled = d["filteringEnabled"].GetBool();
    }
    else
    {
        config.filtering_enabled = true;
    }

    if (d.HasMember("defaultThreshold"))
    {
        config.default_threshold = ToLogLevelT(d["defaultThreshold"].GetString());
    }
    else if (d.HasMember("defaultThresold"))
    {
        config.default_threshold = ToLogLevelT(d["defaultThresold"].GetString());
    }
    else
    {
        std::cerr << "No defaultThreshold or defaultThresold found, set to kVerbose by default" << std::endl;
        config.default_threshold = mw::log::LogLevel::kVerbose;
    }

    const auto& thresholds = d["messageThresholds"];
    for (auto itr1 = thresholds.MemberBegin(); itr1 != thresholds.MemberEnd(); ++itr1)
    {
        const DltidT app_id(itr1->name.GetString());
        const auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const DltidT ctx_id(itr2->name.GetString());
            config.message_thresholds[app_id][ctx_id] = ToLogLevelT(itr2->value.GetString());
        }
    }

    if (d.HasMember("quotas"))
    {
        const std::string quota_enforcement_enabled_param_name = "quotaEnforcementEnabled";
        if (d["quotas"].HasMember(quota_enforcement_enabled_param_name.c_str()))
        {
            config.quota_enforcement_enabled = d["quotas"][quota_enforcement_enabled_param_name.c_str()].GetBool();
        }
        else
        {
            config.quota_enforcement_enabled = false;
        }

        const auto& throughput = d["quotas"]["throughput"];
        config.throughput.overall_mbps = throughput["overallMbps"].GetDouble();
        const auto& applications_kbps = throughput["applicationsKbps"];
        for (auto itr1 = applications_kbps.MemberBegin(); itr1 != applications_kbps.MemberEnd(); ++itr1)
        {
            config.throughput.applications_kbps.emplace(DltidT(itr1->name.GetString()), itr1->value.GetDouble());
        }
    }
    return config;
}

score::logging::dltserver::PersistentConfig ReadDlt(IPersistentDictionary& pd)
{
    using rapidjson::Document;
    using rapidjson::ParseResult;
    score::logging::dltserver::PersistentConfig config{};

    const std::string json = pd.GetString(kConfigDatabaseKey, "{}");

    Document d = CreateRjDocument();
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
        const auto* const name = itr->name.GetString();
        const auto threshold = ToLogLevelT(itr->value["channelThreshold"].GetString());
        score::logging::dltserver::PersistentConfig::ChannelDescription channel{threshold};
        config.channels.emplace(name, channel);
    }

    const auto& assignments = d["channelAssignments"];
    for (auto itr1 = assignments.MemberBegin(); itr1 != assignments.MemberEnd(); ++itr1)
    {
        const DltidT app_id(itr1->name.GetString());
        const auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const DltidT ctx_id(itr2->name.GetString());
            const auto& assigned = itr2->value;
            for (unsigned int itr3 = 0; itr3 < assigned.Size(); ++itr3)
            {
                config.channel_assignments[app_id][ctx_id].push_back(DltidT(assigned[itr3].GetString()));
            }
        }
    }

    if (d.HasMember("filteringEnabled"))
    {
        config.filtering_enabled = d["filteringEnabled"].GetBool();
    }
    else
    {
        config.filtering_enabled = true;
    }
    // TODO: fix typo
    config.default_threshold = ToLogLevelT(d["defaultThresold"].GetString());

    const auto& thresholds = d["messageThresholds"];
    for (auto itr1 = thresholds.MemberBegin(); itr1 != thresholds.MemberEnd(); ++itr1)
    {
        const DltidT app_id(itr1->name.GetString());
        const auto& contexts = itr1->value;
        for (auto itr2 = contexts.MemberBegin(); itr2 != contexts.MemberEnd(); ++itr2)
        {
            const DltidT ctx_id(itr2->name.GetString());
            config.message_thresholds[app_id][ctx_id] = ToLogLevelT(itr2->value.GetString());
        }
    }

    return config;
}

void WriteDlt(const score::logging::dltserver::PersistentConfig& config, IPersistentDictionary& pd)
{
    using rapidjson::Document;
    using rapidjson::StringBuffer;
    using rapidjson::Value;
    using rapidjson::Writer;
    Document d = CreateRjDocument();
    d.SetObject();
    auto& allocator = d.GetAllocator();

    // channels
    Value channels(rapidjson::kObjectType);
    for (const auto& channel : config.channels)
    {
        const std::string& channel_name = channel.first;
        const std::string& channel_threshold = GetStringFromLogLevelT(channel.second.channel_threshold);
        Value channel_json(rapidjson::kObjectType);
        channel_json.AddMember("channelThreshold", Value(channel_threshold.c_str(), allocator).Move(), allocator);
        channels.AddMember(Value(channel_name.c_str(), allocator).Move(), channel_json.Move(), allocator);
    }
    d.AddMember("channels", channels.Move(), allocator);

    // channel assignments
    Value r_assignments(rapidjson::kObjectType);
    for (const auto& app : config.channel_assignments)
    {
        const std::string& app_id = std::string(app.first);
        const auto& contexts = app.second;
        Value r_contexts{rapidjson::kObjectType};
        for (const auto& ctx : contexts)
        {
            const std::string& ctx_id = std::string(ctx.first);
            const auto& assigned = ctx.second;
            Value r_channels{rapidjson::kArrayType};
            for (const auto& channel : assigned)
            {
                r_channels.PushBack(Value(std::string{channel}.c_str(), allocator).Move(), allocator);
            }
            r_contexts.AddMember(Value(ctx_id.c_str(), allocator).Move(), r_channels.Move(), allocator);
        }
        r_assignments.AddMember(Value(app_id.c_str(), allocator).Move(), r_contexts.Move(), allocator);
    }
    d.AddMember("channelAssignments", r_assignments.Move(), allocator);

    d.AddMember("filteringEnabled", config.filtering_enabled, allocator);
    // TODO: fix typo
    d.AddMember("defaultThresold",
                Value(GetStringFromLogLevelT(config.default_threshold).c_str(), allocator).Move(),
                allocator);

    Value r_thresholds(rapidjson::kObjectType);
    for (const auto& app : config.message_thresholds)
    {
        const std::string& app_id = std::string(app.first);
        const auto& contexts = app.second;
        Value r_contexts{rapidjson::kObjectType};
        for (const auto& ctx : contexts)
        {
            const std::string& ctx_id = std::string(ctx.first);
            const auto& threshold = GetStringFromLogLevelT(ctx.second);
            r_contexts.AddMember(
                Value(ctx_id.c_str(), allocator).Move(), Value(threshold.c_str(), allocator).Move(), allocator);
        }
        r_thresholds.AddMember(Value(app_id.c_str(), allocator).Move(), r_contexts.Move(), allocator);
    }
    d.AddMember("messageThresholds", r_thresholds.Move(), allocator);

    StringBuffer buffer(nullptr);
    Writer<StringBuffer> writer(buffer, nullptr);
    d.Accept(writer);

    const std::string json = buffer.GetString();
    pd.SetString(kConfigDatabaseKey, json);
}

bool ReadDltEnabled(IPersistentDictionary& pd)
{
    const bool enabled = pd.GetBool(kConfigOutputEnabledKey, true);
    if constexpr (kPersistentConfigFeatureEnabled)
    {
        std::cout << "Loaded output enable = " << enabled << " from KVS" << std::endl;
    }
    return enabled;
}

void WriteDltEnabled(bool enabled, IPersistentDictionary& pd)
{
    pd.SetBool(kConfigOutputEnabledKey, enabled);
}

}  // namespace datarouter
}  // namespace platform
}  // namespace score
