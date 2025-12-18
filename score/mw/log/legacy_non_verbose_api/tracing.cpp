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

#include "score/mw/log/legacy_non_verbose_api/tracing.h"

#include "score/os/unistd.h"
#include "score/mw/log/configuration/nvconfigfactory.h"

#include <iostream>
#include <sstream>

namespace score
{
namespace platform
{
/*
Deviation from Rule A15-5-3
- The rule states: "The std::terminate() function shall not be called implicitly".
Justification:
- Issue comes from score::json::Any (internally used by score::mw::log::NvConfigFactory),
Any()::operator== (std::bad_variant_access exception),
exception is covered by SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD() macro and function is declared as noexcept,
so suppressing this warning
*/
// coverity[autosar_cpp14_a15_5_3_violation]
logger::logger(const score::cpp::optional<const score::mw::log::detail::Configuration>& config,
               std::optional<score::mw::log::NvConfig> nv_config,
               score::cpp::optional<score::mw::log::detail::SharedMemoryWriter>&& writer) noexcept
    : config_(config.has_value() ? config.value() : score::mw::log::detail::Configuration()),
      nvconfig_(nv_config.has_value() ? std::move(nv_config.value()) :
                []() {
                    auto result = score::mw::log::NvConfigFactory::CreateAndInit();
                    return result.has_value() ? std::move(result.value()) : score::mw::log::NvConfigFactory::CreateEmpty();
                }()),
      discard_operation_fallback_shm_data_{},
      discard_operation_fallback_shm_writer_{InitializeSharedData(discard_operation_fallback_shm_data_),
                                             []() noexcept {}},
      appPrefix{}
{
    /*
    Deviation from Rule A18-9-2:
    - Forwarding values to other functions shall be done via: (1) std::move if the value is an rvalue reference, (2)
      std::forward if the value is forwarding reference
    Justification:
    - the writer is not forward to other function
    */
    // coverity[autosar_cpp14_a18_9_2_violation]
    if (writer.has_value())
    {
        // coverity[autosar_cpp14_a18_9_2_violation] see above
        std::ignore = shared_memory_writer_.emplace(std::move(writer.value()));
    }
    constexpr auto idsize = score::mw::log::detail::LoggingIdentifier::kMaxLength;
    std::fill(appPrefix.begin(), appPrefix.end(), 0);
    auto appPrefixIter = appPrefix.begin();
    static_assert(idsize < static_cast<std::size_t>(std::numeric_limits<int32_t>::max()), "Unsupported length!");
    std::advance(appPrefixIter, static_cast<int32_t>(idsize));
    appPrefixIter = std::copy(config_.GetEcuId().begin(), config_.GetEcuId().end(), appPrefixIter);
    std::ignore = std::copy(config_.GetAppId().begin(), config_.GetAppId().end(), appPrefixIter);
}

std::optional<LogLevel> logger::GetLevelForContext(const std::string& name) const noexcept
{
    const score::mw::log::config::NvMsgDescriptor* const msg_desc = nvconfig_.getDltMsgDesc(name);
    if (msg_desc != nullptr)
    {
        const auto ctxId = msg_desc->GetCtxId();
        const auto context_log_level_map = config_.GetContextLogLevel();
        const auto context = context_log_level_map.find(ctxId);
        if (context != context_log_level_map.end())
        {
            return static_cast<LogLevel>(context->second);
        }
    }
    return std::nullopt;
}

logger& logger::instance(const score::cpp::optional<const score::mw::log::detail::Configuration>& config,
                         std::optional<score::mw::log::NvConfig> nv_config,
                         score::cpp::optional<score::mw::log::detail::SharedMemoryWriter> writer) noexcept
{
    if (*GetInjectedTestInstance() != nullptr)
    {
        return **GetInjectedTestInstance();
    }
    // It's a singleton by design hence cannot be made const
    // coverity[autosar_cpp14_a3_3_2_violation]
    static logger logger_instance{config, std::move(nv_config), std::move(writer)};
    return logger_instance;
}

logger** logger::GetInjectedTestInstance()
{
    static logger* pointer{nullptr};
    return &pointer;
}

void logger::InjectTestInstance(logger* const logger_ptr)
{
    *GetInjectedTestInstance() = logger_ptr;
}

const score::mw::log::detail::Configuration& logger::get_config() const
{
    return config_;
}

score::mw::log::detail::SharedMemoryWriter& logger::GetSharedMemoryWriter()
{
    if (shared_memory_writer_.has_value())
    {
        return shared_memory_writer_.value();
    }
    //  return a fallback that will discard any operations requested as a way of dealing with logging operation
    //  failures. This approach is used to avoid application abort.
    // Using getter method and return the reference to avoid copy overhead.
    // coverity[autosar_cpp14_a9_3_1_violation]
    return discard_operation_fallback_shm_writer_;
}

}  // namespace platform
}  // namespace score
