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
    const auto appId = desc.GetAppId().GetStringView();
    const auto ctxId = desc.GetCtxId().GetStringView();
    filterAndCall(dltid_t{score::cpp::string_view{appId.data(), appId.size()}},
                  dltid_t{score::cpp::string_view{ctxId.data(), ctxId.size()}},
                  desc.GetLogLevel(),
                  sender);
}

void DltLogServer::sendVerbose(
    uint32_t tmsp,
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry)
{
    const auto sender = [&tmsp, &entry, this](DltLogChannel& c) {
        log_sender_->SendVerbose(tmsp, entry, c);
    };
    filterAndCall(
        platform::convertToDltId(entry.app_id), platform::convertToDltId(entry.ctx_id), entry.log_level, sender);
}

void DltLogServer::sendFTVerbose(score::cpp::span<const std::uint8_t> data,
                                 mw::log::LogLevel loglevel,
                                 dltid_t appId,
                                 dltid_t ctxId,
                                 uint8_t nor,
                                 uint32_t tmsp)
{
    const auto sender = [&data, &loglevel, &appId, &ctxId, &nor, &tmsp, this](DltLogChannel& c) {
        log_sender_->SendFTVerbose(data, loglevel, appId, ctxId, nor, tmsp, c);
    };
    // Coredump channel SIZE_MAX value means that there the configuration settings
    // don't explicitly specify the coredump channel
    if (coredumpChannel_.has_value())
    {
        sender(channels_[coredumpChannel_.value()]);
    }
    else
    {
        filterAndCall(appId, ctxId, loglevel, sender);
    }
}

void DltLogServer::init_log_channels(const bool reloading)
{
    if (staticConfig_.channels.empty())
    {
        std::cerr << "Empty channel list" << std::endl;
        init_log_channels_default(reloading);
        return;
    }
    if (staticConfig_.channels.size() >= channelmask_t{}.size())
    {
        std::cerr << "Channel list too long" << std::endl;
        init_log_channels_default(reloading);
        return;
    }

    coredumpChannel_ = std::nullopt;
    PersistentConfig config{readerCallback_()};
    const bool hasPersistentConfig = !config.channels.empty();

    // channels
    if (reloading)
    {
        for (auto& channel : channels_)
        {
            const auto name = channel.channelName_;
            const loglevel_t threshold = hasPersistentConfig ? config.channels[std::string(name)].channelThreshold
                                                             : staticConfig_.channels[name].channelThreshold;
            channel.channelThreshold_.store(threshold, std::memory_order_relaxed);
        }
    }
    else
    {
        const auto& channels = staticConfig_.channels;
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
            if (staticConfig_.defaultChannel == name)
            {
                defaultChannel_ = i;
            }
            if (staticConfig_.coredumpChannel == name)
            {
                coredumpChannel_ = i;
            }
            const loglevel_t threshold =
                hasPersistentConfig ? config.channels[std::string(name)].channelThreshold : channel.channelThreshold;
            const auto ecu = channel.ecu;
            const auto addr = channel.address.c_str();
            const auto port = channel.port;
            const auto dstAddress = channel.dstAddress.empty() ? "239.255.42.99" : channel.dstAddress.c_str();
            auto dstPort = channel.dstPort != 0 ? channel.dstPort : 3490U;
            const auto multicastInterface = channel.multicastInterface.c_str();
            channels_.emplace_back(name, threshold, ecu, addr, port, dstAddress, dstPort, multicastInterface);
            channelNums_[name] = i;
        }
    }

    channelAssignments_.clear();
    const auto& assignments = hasPersistentConfig ? config.channelAssignments : staticConfig_.channelAssignments;
    for (const auto& app : assignments)
    {
        const dltid_t appId = app.first;
        const auto& contexts = app.second;
        for (const auto& ctx : contexts)
        {
            const dltid_t ctxId = ctx.first;
            const auto& channels = ctx.second;
            channelmask_t channelSet{};
            for (const auto& channel : channels)
            {
                const size_t channelNum = channelNums_[dltid_t{channel}];
                channelSet |= channelmask_t{1U} << channelNum;
            }
            channelAssignments_.emplace(std::make_pair(appId, ctxId), channelSet);
        }
    }

    filteringEnabled_ = hasPersistentConfig ? config.filteringEnabled : staticConfig_.filteringEnabled;

    const loglevel_t defaultThreshold = hasPersistentConfig ? config.defaultThreshold : staticConfig_.defaultThreshold;
    defaultThreshold_ = defaultThreshold;

    messageThresholds_.clear();
    const auto& thresholds = hasPersistentConfig ? config.messageThresholds : staticConfig_.messageThresholds;
    for (const auto& app : thresholds)
    {
        const dltid_t appId = app.first;
        const auto& contexts = app.second;
        for (const auto& ctx : contexts)
        {
            const dltid_t ctxId = ctx.first;
            const loglevel_t threshold = ctx.second;
            messageThresholds_.emplace(std::make_pair(appId, ctxId), threshold);
        }
    }

    throughput_overall_ = staticConfig_.throughput.overallMbps;
    throughput_apps_.clear();
    for (const auto& app : staticConfig_.throughput.applicationsKbps)
    {
        const dltid_t appId = app.first;
        const auto& kbps = app.second;
        throughput_apps_.emplace(appId, kbps);
    }
}

void DltLogServer::init_log_channels_default(const bool reloading)
{
    filteringEnabled_ = false;
    defaultThreshold_ = mw::log::LogLevel::kError;
    defaultChannel_ = 0;
    coredumpChannel_ = std::nullopt;
    if (reloading)
    {
        channels_[0].channelThreshold_.store(mw::log::LogLevel::kOff, std::memory_order_relaxed);
    }
    else
    {
        channels_.emplace_back("TEST", mw::log::LogLevel::kInfo, "HOST", "0.0.0.0", 3491, "239.255.42.99", 3490, "");
    }
}

void DltLogServer::set_output_enabled(const bool enabled)
{
    const bool update = (dltOutputEnabled_ != enabled);

    if (update)
    {
        dltOutputEnabled_ = enabled;
        if (enabledCallback_)
        {
            enabledCallback_(enabled);
        }
    }
}
bool DltLogServer::GetDltEnabled() const noexcept
{
    return dltOutputEnabled_;
}

void DltLogServer::save_database()
{
    PersistentConfig config;

    for (auto& channel : channels_)
    {
        config.channels[std::string(channel.channelName_)].channelThreshold = channel.channelThreshold_.load();
    }

    for (auto& assignment : channelAssignments_)
    {
        const dltid_t appId{assignment.first.first};
        const dltid_t ctxId{assignment.first.second};
        const channelmask_t channelSet = assignment.second;
        std::vector<dltid_t> assignments;
        for (size_t i = 0; i < channels_.size(); ++i)
        {
            if (channelSet[i])
            {
                assignments.push_back(channels_[i].channelName_);
            }
        }
        config.channelAssignments[appId][ctxId] = std::move(assignments);
    }

    config.filteringEnabled = filteringEnabled_;
    config.defaultThreshold = defaultThreshold_;

    for (auto& messageThreshold : messageThresholds_)
    {
        const dltid_t appId{messageThreshold.first.first};
        const dltid_t ctxId{messageThreshold.first.second};
        const loglevel_t threshold = messageThreshold.second;
        config.messageThresholds[appId][ctxId] = threshold;
    }

    writerCallback_(std::move(config));
}

void DltLogServer::clear_database()
{
    writerCallback_(PersistentConfig{});
}

const std::string DltLogServer::ReadLogChannelNames()
{
    std::string response(1, config::RET_ERROR);

    std::lock_guard<std::mutex> lock(configMutex_);
    for (auto& channel : channels_)
    {
        appendId(channel.channelName_, response);
    }

    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::ResetToDefault()
{
    std::string response(1, config::RET_ERROR);

    std::lock_guard<std::mutex> lock(configMutex_);
    clear_database();
    init_log_channels(true);

    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::StoreDltConfig()
{
    std::string response(1, config::RET_ERROR);

    std::lock_guard<std::mutex> lock(configMutex_);
    save_database();

    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetTraceState()
{
    std::string response(1, config::RET_ERROR);

    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetDefaultTraceState()
{
    std::string response(1, config::RET_ERROR);

    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetLogChannelThreshold(dltid_t channel, loglevel_t threshold)
{
    std::string response(1, config::RET_ERROR);

    std::lock_guard<std::mutex> lock(configMutex_);
    auto channelIt = channelNums_.find(channel);
    if (channelIt == channelNums_.end())
    {
        response[0] = config::RET_ERROR;
        return response;
    }

    channels_[channelIt->second].channelThreshold_.store(threshold, std::memory_order_relaxed);
    // Trace state (command[1 + 4 + 1] is ignored for now
    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetLogLevel(dltid_t appId, dltid_t ctxId, threshold_t threshold)
{
    std::string response(1, config::RET_ERROR);

    std::lock_guard<std::mutex> lock(configMutex_);
    messageThresholds_.erase({appId, ctxId});
    if (std::holds_alternative<loglevel_t>(threshold))
    {
        messageThresholds_.emplace(std::make_pair(appId, ctxId), std::get<loglevel_t>(threshold));
    }
    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetMessagingFilteringState(bool enabled)
{
    std::string response(1, config::RET_ERROR);

    std::lock_guard<std::mutex> lock(configMutex_);
    filteringEnabled_ = enabled;
    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetDefaultLogLevel(loglevel_t level)
{
    std::string response(1, config::RET_ERROR);

    std::lock_guard<std::mutex> lock(configMutex_);
    defaultThreshold_ = static_cast<loglevel_t>(level);
    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetLogChannelAssignment(dltid_t appId,
                                                        dltid_t ctxId,
                                                        dltid_t channel,
                                                        AssignmentAction assignment_flag)
{
    std::string response(1, config::RET_ERROR);

    auto channelIt = channelNums_.find(channel);
    if (channelIt == channelNums_.end())
    {
        response[0] = config::RET_ERROR;
        return response;
    }

    auto mask = channelmask_t{1U} << channelIt->second;

    std::lock_guard<std::mutex> lock(configMutex_);
    auto cap = channelAssignments_.find({appId, ctxId});
    if (cap == channelAssignments_.end())
    {
        if (assignment_flag == AssignmentAction::Add)
        {
            channelAssignments_.emplace(std::make_pair(appId, ctxId), mask);
        }
    }
    else
    {
        if (assignment_flag == AssignmentAction::Add)
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
                channelAssignments_.erase(cap);  // LCOV_EXCL_LINE
            }
        }
    }
    response[0] = config::RET_OK;
    return response;
}

const std::string DltLogServer::SetDltOutputEnable(bool enable)
{
    std::string response(1, config::RET_ERROR);
    // Justification: Explicit bool cast required to avoid implicit-conversion warning.
    // coverity[misra_cpp_2023_rule_7_0_2_violation]
    if (enable == static_cast<bool>(config::DISABLE))
    {
        score::mw::log::LogError() << "DRCMD: disable output";
        set_output_enabled(false);
        response[0] = config::RET_OK;
        return response;
    }
    else
    {
        score::mw::log::LogInfo() << "DRCMD: enable output";
        set_output_enabled(true);
        response[0] = config::RET_OK;
        return response;
    }
}

const std::string DltLogServer::on_config_command(const std::string& command)
{
    std::unique_ptr<IDiagnosticJobHandler> cmd = parser_->parse(command);  // Parsing the command
    if (cmd == nullptr)
    {
        const std::string response(1, config::RET_ERROR);
        return response;
    }
    else
    {
        const auto response = cmd->execute(*this);  // Handling the diagnostic job
        return response;
    }
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
