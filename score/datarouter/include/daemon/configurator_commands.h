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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_CONFIGURATOR_COMMANDS_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_CONFIGURATOR_COMMANDS_H

#include <cstdint>

namespace score
{
namespace logging
{
namespace dltserver
{
namespace config
{
static constexpr std::uint8_t READ_LOG_CHANNEL_NAMES = 8;
static constexpr std::uint8_t SET_LOG_LEVEL = 0;
static constexpr std::uint8_t RESET_TO_DEFAULT = 1;
static constexpr std::uint8_t SET_MESSAGING_FILTERING_STATE = 2;
static constexpr std::uint8_t SET_LOG_CHANNEL_THRESHOLD = 3;
static constexpr std::uint8_t STORE_DLT_CONFIG = 4;
static constexpr std::uint8_t SET_TRACE_STATE = 5;
static constexpr std::uint8_t SET_DEFAULT_LOG_LEVEL = 6;
static constexpr std::uint8_t SET_DEFAULT_TRACE_STATE = 7;
static constexpr std::uint8_t SET_LOG_CHANNEL_ASSIGNMENT = 9;
static constexpr std::uint8_t SET_DLT_OUTPUT_ENABLE = 10;

static constexpr std::uint8_t DLT_ASSIGN_ADD = 1;

static constexpr char RET_OK = static_cast<char>(0);
static constexpr char RET_UNKNOWN = static_cast<char>(-1);
static constexpr char RET_ERROR = static_cast<char>(-2);

static constexpr std::uint8_t ENABLE = 1;
static constexpr std::uint8_t DISABLE = 0;
}  // namespace config
}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_CONFIGURATOR_COMMANDS_H
