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

score::platform::DltidT ExtractId(const std::string& message, const size_t offset)
{
    auto it = message.begin();
    if (offset > std::numeric_limits<std::string::difference_type>::max())
    {
        std::cerr << "Error: Offset is too large for signed conversion\n";
        return {};
    }
    std::advance(it, static_cast<std::string::difference_type>(offset));
    const auto* src = std::addressof(*it);
    score::platform::DltidT id(src);
    return id;
}

void AppendId(score::platform::DltidT name, std::string& message)
{
    std::string chunk = static_cast<std::string>(name).substr(0, 4);  //  Take only up to 4 characters
    chunk.resize(4, '\0');
    message.append(chunk);
}

std::unique_ptr<IDiagnosticJobHandler> DiagnosticJobParser::Parse(const std::string& command)
{
    if (command.empty())
    {
        return nullptr;
    }

    const auto command_id = static_cast<uint8_t>(command[0]);

    switch (command_id)
    {
        case config::kReadLogChannelNames:
        {
            return std::make_unique<ReadLogChannelNamesHandler>();
        }
        case config::kResetToDefault:
        {
            return std::make_unique<ResetToDefaultHandler>();
        }
        case config::kStoreDltConfig:
        {
            return std::make_unique<StoreDltConfigHandler>();
        }
        case config::kSetTraceState:
        {
            return std::make_unique<SetTraceStateHandler>();
        }
        case config::kSetDefaultTraceState:
        {
            return std::make_unique<SetDefaultTraceStateHandler>();
        }
        case config::kSetLogChannelThreshold:
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
                    ExtractId(command, kDiagnosticCommandSize), read_level.value());
                return diag_job_handler;
            }
            else
            {
                std::cerr << "Incorrect value of log level received from diagnostics" << std::endl;
                return nullptr;
            }
            // Trace state (command[kDiagnosticCommandSize + kChannelIdSize + kLogLevelSize] is ignored for now
        }

        case config::kSetLogLevel:
        {

            if (command.size() != kDiagnosticCommandSize + kAppIdSize + kCtxIdSize + kLogLevelSize)
            {
                return nullptr;
            }

            auto threshold = static_cast<uint8_t>(command[kDiagnosticCommandSize + kAppIdSize + kCtxIdSize]);

            if (threshold == static_cast<std::uint8_t>(ThresholdCmd::kUseDefault))
            {
                // LCOV_EXCL_START : see below
                // tooling issue, this part is covered by unit tests DiagnosticJobParserTest::SetLogLevel_OK_UseDefault.
                // For the quality team argumentation, kindly, check Ticket-200702 and Ticket-229594.
                auto diag_job_handler =
                    std::make_unique<SetLogLevelHandler>(ExtractId(command, kDiagnosticCommandSize),
                                                         ExtractId(command, kDiagnosticCommandSize + kAppIdSize),
                                                         ThresholdCmd::kUseDefault);
                // LCOV_EXCL_STOP
                return diag_job_handler;
            }
            else
            {
                const auto read_level = mw::log::TryGetLogLevelFromU8(threshold);
                if (read_level.has_value())
                {
                    auto diag_job_handler =
                        std::make_unique<SetLogLevelHandler>(ExtractId(command, kDiagnosticCommandSize),
                                                             ExtractId(command, kDiagnosticCommandSize + kAppIdSize),
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

        case config::kSetMessagingFilteringState:
        {

            if (command.size() != kDiagnosticCommandSize + kStateSize)
            {
                return nullptr;
            }

            auto diag_job_handler =
                std::make_unique<SetMessagingFilteringStateHandler>((command[kDiagnosticCommandSize] != 0));

            return diag_job_handler;
        }

        case config::kSetDefaultLogLevel:
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

        case config::kSetLogChannelAssignment:
        {
            if (command.size() != kDiagnosticCommandSize + kAppIdSize + kCtxIdSize + kChannelIdSize + kLogLevelSize)
            {
                return nullptr;
            }

            const auto action_byte = static_cast<std::uint8_t>(
                static_cast<unsigned char>(command[kDiagnosticCommandSize + kAppIdSize + kCtxIdSize + kChannelIdSize]));

            const auto action_opt = GetAssignmentAction(action_byte);
            if (!action_opt.has_value())
            {
                std::cerr << "Incorrect value of assignment received from diagnostics" << std::endl;
                return nullptr;
            }

            auto diag_job_handler = std::make_unique<SetLogChannelAssignmentHandler>(
                ExtractId(command, kDiagnosticCommandSize),
                ExtractId(command, kDiagnosticCommandSize + kAppIdSize),
                ExtractId(command, kDiagnosticCommandSize + kAppIdSize + kCtxIdSize),
                action_opt.value());

            return diag_job_handler;
        }

        case config::kSetDltOutputEnable:
        {
            if (command.size() != kDiagnosticCommandSize + kStateSize)
            {
                return nullptr;
            }
            auto flag = static_cast<uint8_t>(command[kDiagnosticCommandSize]);
            if (flag != config::kEnable && flag != config::kDisable)
            {
                return nullptr;
            }

            auto diag_job_handler = std::make_unique<SetDltOutputEnableHandler>(flag == config::kEnable);

            return diag_job_handler;
        }

        default:
            // Command ID is not recognized
            return nullptr;
    }
}

std::optional<AssignmentAction> DiagnosticJobParser::GetAssignmentAction(std::uint8_t value) noexcept
{
    if (value <= static_cast<std::uint8_t>(AssignmentAction::kAdd))
    {
        return static_cast<AssignmentAction>(value);
    }
    return std::nullopt;
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
