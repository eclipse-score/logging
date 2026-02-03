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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_DLT_LOG_SERVER_CONFIG_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_DLT_LOG_SERVER_CONFIG_H

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

using LoglevelT = mw::log::LogLevel;

struct StaticConfig
{
    struct ChannelDescription
    {
        DltidT ecu;
        std::string address;
        uint16_t port = 0U;
        std::string dst_address;
        uint16_t dst_port = 0U;

        LoglevelT channel_threshold = LoglevelT::kOff;
        std::string multicast_interface;
    };

    struct ThroughputQuotas
    {
        double overall_mbps;
        std::unordered_map<DltidT, double> applications_kbps;
    };

    DltidT coredump_channel;
    DltidT default_channel;
    std::unordered_map<DltidT, ChannelDescription> channels;

    bool filtering_enabled = false;
    LoglevelT default_threshold = LoglevelT::kOff;
    std::unordered_map<DltidT, std::unordered_map<DltidT, std::vector<DltidT>>> channel_assignments;
    std::unordered_map<DltidT, std::unordered_map<DltidT, LoglevelT>> message_thresholds;

    ThroughputQuotas throughput;
    bool quota_enforcement_enabled;
};

struct PersistentConfig
{
    struct ChannelDescription
    {
        LoglevelT channel_threshold = LoglevelT::kOff;
    };
    std::unordered_map<std::string, ChannelDescription> channels;

    bool filtering_enabled = false;
    LoglevelT default_threshold = LoglevelT::kOff;
    std::unordered_map<DltidT, std::unordered_map<DltidT, std::vector<DltidT>>> channel_assignments;
    std::unordered_map<DltidT, std::unordered_map<DltidT, LoglevelT>> message_thresholds;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_DLT_LOG_SERVER_CONFIG_H
