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

#include "score/mw/log/detail/data_router/data_router_backend.h"

#include "score/mw/log/legacy_non_verbose_api/tracing.h"

#include <score/utility.hpp>
#include <iostream>
#include <type_traits>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

namespace
{

std::size_t CheckForMaxCapacity(const std::size_t capacity) noexcept
{
    const auto is_within_max_capacity = (capacity <= std::numeric_limits<SlotIndex>::max());
    if (is_within_max_capacity)
    {
        return capacity;
    }
    else
    {
        // Cast from unsigned char to std::size_t is valid. To prevent implicit conversion.
        return static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max());
    }
}

}  //  namespace

DataRouterBackend::DataRouterBackend(const std::size_t number_of_slots,
                                     const LogRecord& initial_slot_value,
                                     DatarouterMessageClientFactory& message_client_factory,
                                     const Configuration& config,
                                     WriterFactory writer_factory)
    : Backend{}, buffer_{CheckForMaxCapacity(number_of_slots), initial_slot_value}, message_client_{nullptr}
{

    auto writer =
        writer_factory.Create(config.GetRingBufferSize(), config.GetDynamicDatarouterIdentifiers(), config.GetAppId());

    // start running and create the logger and message client factory only if writer is having value.
    if (writer.has_value())
    {
        // Required to retrieve and update the Configuration across
        std::ignore = ::score::platform::logger::instance(config, {}, std::move(writer.value()));
        message_client_ =
            message_client_factory.CreateOnce(writer_factory.GetIdentifier(), writer_factory.GetFileName());
        message_client_->Run();
    }
}

score::cpp::optional<SlotHandle> DataRouterBackend::ReserveSlot() noexcept
{
    const auto& slot = buffer_.AcquireSlotToWrite();
    if (slot.has_value())
    {
        //  CircularAllocator has capacity limited by CheckFoxMaxCapacity thus the cast is valid:
        // We intentionally static casting to SlotIndex(uint8_t)
        // to limit memory allocations to the required levels during startup and
        // since there is no need to support slots greater than uint8 as per the current system needs.
        // coverity[autosar_cpp14_a4_7_1_violation]
        return SlotHandle{static_cast<SlotIndex>(slot.value())};
    }
    else
    {
        return {};
    }
}

LogRecord& DataRouterBackend::GetLogRecord(const SlotHandle& slot) noexcept
{
    // Cast from std::uint8_t to std::size_t is valid. To prevent implicit conversion.
    return buffer_.GetUnderlyingBufferFor(static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder()));
}

void DataRouterBackend::FlushSlot(const SlotHandle& slot) noexcept
{
    // Cast from std::uint8_t to std::size_t is valid. To prevent implicit conversion.
    auto& log_entry =
        buffer_.GetUnderlyingBufferFor(static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder())).getLogEntry();

    switch (log_entry.log_level)
    {
        case LogLevel::kVerbose:
            TRACE_VERBOSE(log_entry);
            break;
        case LogLevel::kDebug:
            TRACE_DEBUG(log_entry);
            break;
        case LogLevel::kInfo:
            TRACE_INFO(log_entry);
            break;
        case LogLevel::kWarn:
            TRACE_WARN(log_entry);
            break;
        case LogLevel::kError:
            TRACE_ERROR(log_entry);
            break;
        case LogLevel::kFatal:
            TRACE_FATAL(log_entry);
            break;
        case LogLevel::kOff:
        default:
            break;
    }

    // Cast from std::uint8_t to std::size_t is valid. To prevent implicit conversion.
    buffer_.ReleaseSlot(static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder()));
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
