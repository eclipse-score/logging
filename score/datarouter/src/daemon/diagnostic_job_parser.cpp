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

#include "score/datarouter/include/daemon/diagnostic_job_parser.h"
#include "score/datarouter/include/daemon/configurator_commands.h"
#include "score/datarouter/include/daemon/diagnostic_job_handler.h"

namespace score
{
namespace logging
{
namespace dltserver
{

constexpr auto kDiagnosticCommandSize{1UL};
constexpr auto kLogLevelSize{1UL};
constexpr auto kAppIdSize{4UL};
constexpr auto kCtxIdSize{4UL};
constexpr auto kChannelIdSize{4UL};
constexpr auto kTraceStateId{1UL};
constexpr auto kStateSize{1UL};

score::platform::dltid_t extractId(const std::string& message, const size_t offset)
{
    auto it = message.begin();
    if (offset > std::numeric_limits<std::string::difference_type>::max())
    {
        std::cerr << "Error: Offset is too large for signed conversion\n";
        return {};
    }
    std::advance(it, static_cast<std::string::difference_type>(offset));
    auto* src = std::addressof(*it);
    score::platform::dltid_t id(src);
    return id;
}

void appendId(score::platform::dltid_t name, std::string& message)
{
    std::string chunk = static_cast<std::string>(name).substr(0, 4);  //  Take only up to 4 characters
    chunk.resize(4, '\0');
    message.append(chunk);
}

std::unique_ptr<IDiagnosticJobHandler> DiagnosticJobParser::parse(const std::string& command)
{
    if (command.empty())
    {
        return nullptr;
    }

    const auto commandId = static_cast<uint8_t>(command[0]);

    switch (commandId)
    {
        case config::READ_LOG_CHANNEL_NAMES:
        {
            return std::make_unique<ReadLogChannelNamesHandler>();
        }
        case config::RESET_TO_DEFAULT:
        {
            return std::make_unique<ResetToDefaultHandler>();
        }
        case config::STORE_DLT_CONFIG:
        {
            return std::make_unique<StoreDltConfigHandler>();
        }
        case config::SET_TRACE_STATE:
        {
            return std::make_unique<SetTraceStateHandler>();
        }
        case config::SET_DEFAULT_TRACE_STATE:
        {
            return std::make_unique<SetDefaultTraceStateHandler>();
        }
        case config::SET_LOG_CHANNEL_THRESHOLD:
        {

            if (command.size() != kDiagnosticCommandSize + kChannelIdSize + kLogLevelSize + kTraceStateId)
            {
                return nullptr;
            }

            const auto read_level =
                mw::log::TryGetLogLevelFromU8(static_cast<uint8_t>(command[kDiagnosticCommandSize + kChannelIdSize]));
            if (read_level.has_value())
            {
                auto diag_job_handler = std::make_unique<SetLogChannelThresholdHandler>(
                    extractId(command, kDiagnosticCommandSize), read_level.value());
                return diag_job_handler;
            }
            else
            {
                std::cerr << "Incorrect value of log level received from diagnostics" << std::endl;
                return nullptr;
            }
            // Trace state (command[kDiagnosticCommandSize + kChannelIdSize + kLogLevelSize] is ignored for now
        }

        case config::SET_LOG_LEVEL:
        {

            if (command.size() != kDiagnosticCommandSize + kAppIdSize + kCtxIdSize + kLogLevelSize)
            {
                return nullptr;
            }

            auto threshold = static_cast<uint8_t>(command[kDiagnosticCommandSize + kAppIdSize + kCtxIdSize]);

            if (threshold == static_cast<std::uint8_t>(ThresholdCmd::UseDefault))
            {
                // LCOV_EXCL_START : see below
                // tooling issue, this part is covered by unit tests DiagnosticJobParserTest::SetLogLevel_OK_UseDefault.
                // For the quality team argumentation, kindly, check Ticket-200702 and Ticket-229594.
                auto diag_job_handler =
                    std::make_unique<SetLogLevelHandler>(extractId(command, kDiagnosticCommandSize),
                                                         extractId(command, kDiagnosticCommandSize + kAppIdSize),
                                                         ThresholdCmd::UseDefault);
                // LCOV_EXCL_STOP
                return diag_job_handler;
            }
            else
            {
                const auto read_level = mw::log::TryGetLogLevelFromU8(threshold);
                if (read_level.has_value())
                {
                    auto diag_job_handler =
                        std::make_unique<SetLogLevelHandler>(extractId(command, kDiagnosticCommandSize),
                                                             extractId(command, kDiagnosticCommandSize + kAppIdSize),
                                                             read_level.value());
                    return diag_job_handler;
                }
                else
                {
                    std::cerr << "Incorrect value of log level received from diagnostics" << std::endl;
                    return nullptr;
                }
            }
        }

        case config::SET_MESSAGING_FILTERING_STATE:
        {

            if (command.size() != kDiagnosticCommandSize + kStateSize)
            {
                return nullptr;
            }

            auto diag_job_handler =
                std::make_unique<SetMessagingFilteringStateHandler>((command[kDiagnosticCommandSize] != 0));

            return diag_job_handler;
        }

        case config::SET_DEFAULT_LOG_LEVEL:
        {
            if (command.size() != kDiagnosticCommandSize + kLogLevelSize)
            {
                return nullptr;
            }

            const auto read_level =
                mw::log::TryGetLogLevelFromU8(static_cast<uint8_t>(command[kDiagnosticCommandSize]));

            if (read_level.has_value())
            {
                auto diag_job_handler = std::make_unique<SetDefaultLogLevelHandler>(read_level.value());

                return diag_job_handler;
            }
            else
            {
                std::cerr << "Incorrect value of default log level received from diagnostics" << std::endl;

                return nullptr;
            }
        }

        case config::SET_LOG_CHANNEL_ASSIGNMENT:
        {
            if (command.size() != kDiagnosticCommandSize + kAppIdSize + kCtxIdSize + kChannelIdSize + kLogLevelSize)
            {
                return nullptr;
            }

            const auto action_byte = static_cast<std::uint8_t>(
                static_cast<unsigned char>(command[kDiagnosticCommandSize + kAppIdSize + kCtxIdSize + kChannelIdSize]));

            const auto action_opt = getAssignmentAction(action_byte);
            if (!action_opt.has_value())
            {
                std::cerr << "Incorrect value of assignment received from diagnostics" << std::endl;
                return nullptr;
            }

            auto diag_job_handler = std::make_unique<SetLogChannelAssignmentHandler>(
                extractId(command, kDiagnosticCommandSize),
                extractId(command, kDiagnosticCommandSize + kAppIdSize),
                extractId(command, kDiagnosticCommandSize + kAppIdSize + kCtxIdSize),
                action_opt.value());

            return diag_job_handler;
        }

        case config::SET_DLT_OUTPUT_ENABLE:
        {
            if (command.size() != kDiagnosticCommandSize + kStateSize)
            {
                return nullptr;
            }
            auto flag = static_cast<uint8_t>(command[kDiagnosticCommandSize]);
            if (flag != config::ENABLE && flag != config::DISABLE)
            {
                return nullptr;
            }

            auto diag_job_handler = std::make_unique<SetDltOutputEnableHandler>(flag == config::ENABLE);

            return diag_job_handler;
        }

        default:
            // Command ID is not recognized
            return nullptr;
    }
}

std::optional<AssignmentAction> DiagnosticJobParser::getAssignmentAction(std::uint8_t value) noexcept
{
    if (value <= static_cast<std::uint8_t>(AssignmentAction::Add))
    {
        return static_cast<AssignmentAction>(value);
    }
    return std::nullopt;
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
