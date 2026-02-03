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

#include "daemon/dlt_log_server.h"

#include "score/datarouter/include/daemon/configurator_commands.h"
#include "score/datarouter/include/daemon/diagnostic_job_parser.h"
#include "score/datarouter/include/daemon/i_diagnostic_job_handler.h"
#include "score/datarouter/include/dlt/dltid_converter.h"

#include <algorithm>
#include <sstream>

#include <iostream>

namespace score
{
namespace logging
{
namespace dltserver
{

void DltLogServer::SendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                                  uint32_t tmsp,
                                  const void* data,
                                  size_t size)
{
    auto sender = [&desc, &tmsp, &data, &size, this](DltLogChannel& c) {
        log_sender_->SendNonVerbose(desc, tmsp, data, size, c);
    };
    const auto app_id = desc.GetAppId().GetStringView();
    const auto ctx_id = desc.GetCtxId().GetStringView();
    FilterAndCall(DltidT{score::cpp::string_view{app_id.data(), app_id.size()}},
                  DltidT{score::cpp::string_view{ctx_id.data(), ctx_id.size()}},
                  desc.GetLogLevel(),
                  sender);
}

void DltLogServer::SendVerbose(
    uint32_t tmsp,
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry)
{
    const auto sender = [&tmsp, &entry, this](DltLogChannel& c) {
        log_sender_->SendVerbose(tmsp, entry, c);
    };
    FilterAndCall(
        platform::ConvertToDltId(entry.app_id), platform::ConvertToDltId(entry.ctx_id), entry.log_level, sender);
}

void DltLogServer::SendFtVerbose(score::cpp::span<const std::uint8_t> data,
                                 mw::log::LogLevel loglevel,
                                 DltidT app_id,
                                 DltidT ctx_id,
                                 uint8_t nor,
                                 uint32_t tmsp)
{
    const auto sender = [&data, &loglevel, &app_id, &ctx_id, &nor, &tmsp, this](DltLogChannel& c) {
        log_sender_->SendFTVerbose(data, loglevel, app_id, ctx_id, nor, tmsp, c);
    };
    // Coredump channel SIZE_MAX value means that there the configuration settings
    // don't explicitly specify the coredump channel
    if (coredump_channel_.has_value())
    {
        sender(channels_[coredump_channel_.value()]);
    }
    else
    {
        FilterAndCall(app_id, ctx_id, loglevel, sender);
    }
}

void DltLogServer::InitLogChannels(const bool reloading)
{
    if (static_config_.channels.empty())
    {
        std::cerr << "Empty channel list" << std::endl;
        InitLogChannelsDefault(reloading);
        return;
    }
    if (static_config_.channels.size() >= ChannelmaskT{}.size())
    {
        std::cerr << "Channel list too long" << std::endl;
        InitLogChannelsDefault(reloading);
        return;
    }

    coredump_channel_ = std::nullopt;
    PersistentConfig config{reader_callback_()};
    const bool has_persistent_config = !config.channels.empty();

    // channels
    if (reloading)
    {
        for (auto& channel : channels_)
        {
            const auto name = channel.channel_name;
            const LoglevelT threshold = has_persistent_config ? config.channels[std::string(name)].channel_threshold
                                                              : static_config_.channels[name].channel_threshold;
            channel.channel_threshold.store(threshold, std::memory_order_relaxed);
        }
    }
    else
    {
        const auto& channels = static_config_.channels;
        size_t i = 0;
        /*
            Deviation from Rule M5-2-10:
            - Rule M5-2-10 (required, implementation, automated)
            The increment (++) and decrement ( ) operators shall not be mixed with
            other operators in an expression.
            Justification:
            - Since there are no other operations besides increment, it is quite clear what is happening.
        */
        // coverity[autosar_cpp14_m5_2_10_violation]
        for (auto itr = channels.begin(); itr != channels.end(); ++itr, ++i)
        {
            const auto& name = itr->first;
            const auto& channel = itr->second;
            if (static_config_.default_channel == name)
            {
                default_channel_ = i;
            }
            if (static_config_.coredump_channel == name)
            {
                coredump_channel_ = i;
            }
            const LoglevelT threshold = has_persistent_config ? config.channels[std::string(name)].channel_threshold
                                                              : channel.channel_threshold;
            const auto ecu = channel.ecu;
            const auto* const addr = channel.address.c_str();
            const auto port = channel.port;
            const auto* const dst_address = channel.dst_address.empty() ? "239.255.42.99" : channel.dst_address.c_str();
            auto dst_port = channel.dst_port != 0 ? channel.dst_port : 3490U;
            const auto* const multicast_interface = channel.multicast_interface.c_str();
            channels_.emplace_back(name, threshold, ecu, addr, port, dst_address, dst_port, multicast_interface);
            channel_nums_[name] = i;
        }
    }

    channel_assignments_.clear();
    const auto& assignments = has_persistent_config ? config.channel_assignments : static_config_.channel_assignments;
    for (const auto& app : assignments)
    {
        const DltidT app_id = app.first;
        const auto& contexts = app.second;
        for (const auto& ctx : contexts)
        {
            const DltidT ctx_id = ctx.first;
            const auto& channels = ctx.second;
            ChannelmaskT channel_set{};
            for (const auto& channel : channels)
            {
                const size_t channel_num = channel_nums_[DltidT{channel}];
                channel_set |= ChannelmaskT{1U} << channel_num;
            }
            channel_assignments_.emplace(std::make_pair(app_id, ctx_id), channel_set);
        }
    }

    filtering_enabled_ = has_persistent_config ? config.filtering_enabled : static_config_.filtering_enabled;

    const LoglevelT default_threshold =
        has_persistent_config ? config.default_threshold : static_config_.default_threshold;
    default_threshold_ = default_threshold;

    message_thresholds_.clear();
    const auto& thresholds = has_persistent_config ? config.message_thresholds : static_config_.message_thresholds;
    for (const auto& app : thresholds)
    {
        const DltidT app_id = app.first;
        const auto& contexts = app.second;
        for (const auto& ctx : contexts)
        {
            const DltidT ctx_id = ctx.first;
            const LoglevelT threshold = ctx.second;
            message_thresholds_.emplace(std::make_pair(app_id, ctx_id), threshold);
        }
    }

    throughput_overall_ = static_config_.throughput.overall_mbps;
    throughput_apps_.clear();
    for (const auto& app : static_config_.throughput.applications_kbps)
    {
        const DltidT app_id = app.first;
        const auto& kbps = app.second;
        throughput_apps_.emplace(app_id, kbps);
    }
}

void DltLogServer::InitLogChannelsDefault(const bool reloading)
{
    filtering_enabled_ = false;
    default_threshold_ = mw::log::LogLevel::kError;
    default_channel_ = 0;
    coredump_channel_ = std::nullopt;
    if (reloading)
    {
        channels_[0].channel_threshold.store(mw::log::LogLevel::kOff, std::memory_order_relaxed);
    }
    else
    {
        channels_.emplace_back("TEST", mw::log::LogLevel::kInfo, "HOST", "0.0.0.0", 3491, "239.255.42.99", 3490, "");
    }
}

void DltLogServer::SetOutputEnabled(const bool enabled)
{
    const bool update = (dlt_output_enabled_ != enabled);

    if (update)
    {
        dlt_output_enabled_ = enabled;
        if (enabled_callback_)
        {
            enabled_callback_(enabled);
        }
    }
}
bool DltLogServer::GetDltEnabled() const noexcept
{
    return dlt_output_enabled_;
}

void DltLogServer::SaveDatabase()
{
    PersistentConfig config;

    for (auto& channel : channels_)
    {
        config.channels[std::string(channel.channel_name)].channel_threshold = channel.channel_threshold.load();
    }

    for (auto& assignment : channel_assignments_)
    {
        const DltidT app_id{assignment.first.first};
        const DltidT ctx_id{assignment.first.second};
        const ChannelmaskT channel_set = assignment.second;
        std::vector<DltidT> assignments;
        for (size_t i = 0; i < channels_.size(); ++i)
        {
            if (channel_set[i])
            {
                assignments.push_back(channels_[i].channel_name);
            }
        }
        config.channel_assignments[app_id][ctx_id] = std::move(assignments);
    }

    config.filtering_enabled = filtering_enabled_;
    config.default_threshold = default_threshold_;

    for (auto& message_threshold : message_thresholds_)
    {
        const DltidT app_id{message_threshold.first.first};
        const DltidT ctx_id{message_threshold.first.second};
        const LoglevelT threshold = message_threshold.second;
        config.message_thresholds[app_id][ctx_id] = threshold;
    }

    writer_callback_(std::move(config));
}

void DltLogServer::ClearDatabase()
{
    writer_callback_(PersistentConfig{});
}

std::string DltLogServer::ReadLogChannelNames()
{
    std::string response(1, config::kRetError);

    std::lock_guard<std::mutex> lock(config_mutex_);
    for (auto& channel : channels_)
    {
        AppendId(channel.channel_name, response);
    }

    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::ResetToDefault()
{
    std::string response(1, config::kRetError);

    std::lock_guard<std::mutex> lock(config_mutex_);
    ClearDatabase();
    InitLogChannels(true);

    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::StoreDltConfig()
{
    std::string response(1, config::kRetError);

    std::lock_guard<std::mutex> lock(config_mutex_);
    SaveDatabase();

    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetTraceState()
{
    std::string response(1, config::kRetError);

    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetDefaultTraceState()
{
    std::string response(1, config::kRetError);

    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetLogChannelThreshold(DltidT channel, LoglevelT threshold)
{
    std::string response(1, config::kRetError);

    std::lock_guard<std::mutex> lock(config_mutex_);
    auto channel_it = channel_nums_.find(channel);
    if (channel_it == channel_nums_.end())
    {
        response[0] = config::kRetError;
        return response;
    }

    channels_[channel_it->second].channel_threshold.store(threshold, std::memory_order_relaxed);
    // Trace state (command[1 + 4 + 1] is ignored for now
    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetLogLevel(DltidT app_id, DltidT ctx_id, ThresholdT threshold)
{
    std::string response(1, config::kRetError);

    std::lock_guard<std::mutex> lock(config_mutex_);
    message_thresholds_.erase({app_id, ctx_id});
    if (std::holds_alternative<LoglevelT>(threshold))
    {
        message_thresholds_.emplace(std::make_pair(app_id, ctx_id), std::get<LoglevelT>(threshold));
    }
    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetMessagingFilteringState(bool enabled)
{
    std::string response(1, config::kRetError);

    std::lock_guard<std::mutex> lock(config_mutex_);
    filtering_enabled_ = enabled;
    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetDefaultLogLevel(LoglevelT level)
{
    std::string response(1, config::kRetError);

    std::lock_guard<std::mutex> lock(config_mutex_);
    default_threshold_ = static_cast<LoglevelT>(level);
    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetLogChannelAssignment(DltidT app_id,
                                                  DltidT ctx_id,
                                                  DltidT channel,
                                                  AssignmentAction assignment_flag)
{
    std::string response(1, config::kRetError);

    auto channel_it = channel_nums_.find(channel);
    if (channel_it == channel_nums_.end())
    {
        response[0] = config::kRetError;
        return response;
    }

    auto mask = ChannelmaskT{1U} << channel_it->second;

    std::lock_guard<std::mutex> lock(config_mutex_);
    auto cap = channel_assignments_.find({app_id, ctx_id});
    if (cap == channel_assignments_.end())
    {
        if (assignment_flag == AssignmentAction::kAdd)
        {
            channel_assignments_.emplace(std::make_pair(app_id, ctx_id), mask);
        }
    }
    else
    {
        if (assignment_flag == AssignmentAction::kAdd)
        {
            cap->second |= mask;
        }
        else
        {
            cap->second &= ~mask;
            if (cap->second.none())
            {
                // Test is available - DltServerWrongChannelsTest, but somehow it is not covered by coverage
                // tool.
                channel_assignments_.erase(cap);  // LCOV_EXCL_LINE
            }
        }
    }
    response[0] = config::kRetOk;
    return response;
}

std::string DltLogServer::SetDltOutputEnable(bool enable)
{
    std::string response(1, config::kRetError);
    // Justification: Explicit bool cast required to avoid implicit-conversion warning.
    // coverity[misra_cpp_2023_rule_7_0_2_violation]
    if (enable == static_cast<bool>(config::kDisable))
    {
        score::mw::log::LogError() << "DRCMD: disable output";
        SetOutputEnabled(false);
        response[0] = config::kRetOk;
        return response;
    }
    else
    {
        score::mw::log::LogInfo() << "DRCMD: enable output";
        SetOutputEnabled(true);
        response[0] = config::kRetOk;
        return response;
    }
}

std::string DltLogServer::OnConfigCommand(const std::string& command)
{
    std::unique_ptr<IDiagnosticJobHandler> cmd = parser_->Parse(command);  // Parsing the command
    if (cmd == nullptr)
    {
        const std::string response(1, config::kRetError);
        return response;
    }
    else
    {
        const auto response = cmd->Execute(*this);  // Handling the diagnostic job
        return response;
    }
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
