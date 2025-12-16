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

#ifndef DLT_LOG_SERVER_CONFIG_H_
#define DLT_LOG_SERVER_CONFIG_H_

#include "daemon/dltserver_common.h"
#include "score/mw/log/log_level.h"

#include <unordered_map>
#include <vector>

namespace score
{
namespace logging
{
namespace dltserver
{

using loglevel_t = mw::log::LogLevel;

struct StaticConfig
{
    struct ChannelDescription
    {
        dltid_t ecu;
        std::string address;
        uint16_t port = 0U;
        std::string dstAddress;
        uint16_t dstPort = 0U;

        loglevel_t channelThreshold = loglevel_t::kOff;
        std::string multicastInterface;
    };

    struct ThroughputQuotas
    {
        double overallMbps;
        std::unordered_map<dltid_t, double> applicationsKbps;
    };

    dltid_t coredumpChannel;
    dltid_t defaultChannel;
    std::unordered_map<dltid_t, ChannelDescription> channels;

    bool filteringEnabled = false;
    loglevel_t defaultThreshold = loglevel_t::kOff;
    std::unordered_map<dltid_t, std::unordered_map<dltid_t, std::vector<dltid_t>>> channelAssignments;
    std::unordered_map<dltid_t, std::unordered_map<dltid_t, loglevel_t>> messageThresholds;

    ThroughputQuotas throughput;
    bool quotaEnforcementEnabled;
};

struct PersistentConfig
{
    struct ChannelDescription
    {
        loglevel_t channelThreshold = loglevel_t::kOff;
    };
    std::unordered_map<std::string, ChannelDescription> channels;

    bool filteringEnabled = false;
    loglevel_t defaultThreshold = loglevel_t::kOff;
    std::unordered_map<dltid_t, std::unordered_map<dltid_t, std::vector<dltid_t>>> channelAssignments;
    std::unordered_map<dltid_t, std::unordered_map<dltid_t, loglevel_t>> messageThresholds;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // DLT_LOG_SERVER_CONFIG_H_
