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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_CONFIGURATOR_COMMANDS_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_CONFIGURATOR_COMMANDS_H

#include <cstdint>

namespace score
{
namespace logging
{
namespace dltserver
{
namespace config
{
static constexpr std::uint8_t kReadLogChannelNames = 8;
static constexpr std::uint8_t kSetLogLevel = 0;
static constexpr std::uint8_t kResetToDefault = 1;
static constexpr std::uint8_t kSetMessagingFilteringState = 2;
static constexpr std::uint8_t kSetLogChannelThreshold = 3;
static constexpr std::uint8_t kStoreDltConfig = 4;
static constexpr std::uint8_t kSetTraceState = 5;
static constexpr std::uint8_t kSetDefaultLogLevel = 6;
static constexpr std::uint8_t kSetDefaultTraceState = 7;
static constexpr std::uint8_t kSetLogChannelAssignment = 9;
static constexpr std::uint8_t kSetDltOutputEnable = 10;

static constexpr std::uint8_t kDltAssignAdd = 1;

static constexpr char kRetOk = static_cast<char>(0);
static constexpr char kRetUnknown = static_cast<char>(-1);
static constexpr char kRetError = static_cast<char>(-2);

static constexpr std::uint8_t kEnable = 1;
static constexpr std::uint8_t kDisable = 0;
}  // namespace config
}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_CONFIGURATOR_COMMANDS_H
