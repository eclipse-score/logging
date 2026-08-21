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
#include "score/mw/log/detail/syslog/syslog_backend.h"

#include "score/mw/log/detail/error.h"
#include "score/mw/log/detail/initialization_reporter.h"

#include <score/assert.hpp>
#include <algorithm>
#include <limits>
#include <tuple>

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

std::size_t CheckTheMaxCapacity(const std::size_t capacity) noexcept
{
    const auto is_within_max_capacity = (capacity <= std::numeric_limits<SlotIndex>::max());
    if (is_within_max_capacity)
    {
        return capacity;
    }
    else
    {
        return static_cast<std::size_t>(std::numeric_limits<SlotIndex>::max());
    }
}

// syslog(3) priorities from <syslog.h>. kInvalid marks a level that must not be emitted
// (e.g. kOff / out-of-range), mirroring slog's kInvalid handling but skipping the emit.
enum class SyslogPriority : std::int32_t
{
    kCrit = LOG_CRIT,
    kErr = LOG_ERR,
    kWarning = LOG_WARNING,
    kInfo = LOG_INFO,
    kDebug = LOG_DEBUG,
    kInvalid = -1
};

constexpr SyslogPriority ConvertMwLogLevelToSyslogPriority(const LogLevel level)
{
    SyslogPriority priority = SyslogPriority::kInvalid;
    switch (level)
    {
        case LogLevel::kVerbose:
            priority = SyslogPriority::kDebug;
            break;
        case LogLevel::kDebug:
            priority = SyslogPriority::kDebug;
            break;
        case LogLevel::kInfo:
            priority = SyslogPriority::kInfo;
            break;
        case LogLevel::kWarn:
            priority = SyslogPriority::kWarning;
            break;
        case LogLevel::kError:
            priority = SyslogPriority::kErr;
            break;
        case LogLevel::kFatal:
            priority = SyslogPriority::kCrit;
            break;
        case LogLevel::kOff:
        default:
            priority = SyslogPriority::kInvalid;
            break;
    }
    return priority;
}

constexpr SyslogPriority ToSyslogPriority(const LogLevel log_level) noexcept
{
    if (log_level <= GetMaxLogLevelValue())
    {
        return ConvertMwLogLevelToSyslogPriority(log_level);
    }
    else
    {
        return SyslogPriority::kInvalid;
    }
}

}  //  namespace

SyslogBackend::SyslogBackend(const std::size_t number_of_slots,
                             const LogRecord& initial_slot_value,
                             const std::string_view app_id,
                             score::cpp::pmr::unique_ptr<score::os::Syslog> syslog_instance) noexcept
    : Backend::Backend(),
      app_id_{app_id.data(), app_id.size()},
      buffer_{CheckTheMaxCapacity(number_of_slots), initial_slot_value},
      syslog_instance_{std::move(syslog_instance)}
{
    Init();
}

score::cpp::optional<SlotHandle> SyslogBackend::ReserveSlot() noexcept
{
    const auto& slot = buffer_.AcquireSlotToWrite();
    if (slot.has_value())
    {
        if (slot.value() < std::numeric_limits<SlotIndex>::max())  // LCOV_EXCL_BR_LINE: As it always true case,we can't
        //  control slot.value() it is received from AcquireSlotToWrite() function
        // which wraps around and resulting in a value within the valid range.
        {
            // CircularAllocator has capacity limited by CheckTheMaxCapacity thus the cast is valid:
            // We intentionally static cast to SlotIndex(uint8_t) to limit memory allocations
            // to the required levels during startup, since there is no need to support slots greater
            // than uint8 as per the current system needs.
            // coverity[autosar_cpp14_a4_7_1_violation]
            return SlotHandle{static_cast<SlotIndex>(slot.value())};
        }
    }
    return {};
}

LogRecord& SyslogBackend::GetLogRecord(const SlotHandle& slot) noexcept
{
    // static cast from std::uint8_t to std::size_t
    return buffer_.GetUnderlyingBufferFor(static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder()));
}

void SyslogBackend::FlushSlot(const SlotHandle& slot) noexcept
{
    // static cast from std::uint8_t to std::size_t
    auto& log_entry =
        buffer_.GetUnderlyingBufferFor(static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder())).GetLogEntry();

    constexpr std::size_t kMaxIdLength{4U};

    // Cast appid length to int32 without overflow.
    const std::int32_t app_id_length = static_cast<int32_t>(std::min(kMaxIdLength, app_id_.size()));

    // Cast context length to int32 without overflow.
    const std::int32_t ctx_id_length =
        static_cast<int32_t>(std::min(kMaxIdLength, log_entry.ctx_id.GetStringView().size()));

    // Cast payload size to int32_t without overflow.
    const std::int32_t payload_length = static_cast<int32_t>(
        std::min(static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()), log_entry.payload.size()));

    const auto priority = ToSyslogPriority(log_entry.log_level);
    if (priority != SyslogPriority::kInvalid)
    {
        // Log message with appid and ctxid.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) no available alternative for syslog
        syslog_instance_->syslog(static_cast<std::int32_t>(priority),
                                 "%.*s,%.*s: %.*s",
                                 app_id_length,
                                 app_id_.c_str(),
                                 ctx_id_length,
                                 // above variable `ctx_id_length` contains corresponding length information
                                 // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage) justified above
                                 log_entry.ctx_id.GetStringView().data(),
                                 payload_length,
                                 log_entry.payload.data());
    }

    buffer_.ReleaseSlot(static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder()));
}

void SyslogBackend::Init() noexcept
{
    // glibc openlog(3) stores the `ident` pointer (it does not copy the string); app_id_ is a
    // member and outlives every syslog() call, so passing its c_str() is safe.
    syslog_instance_->openlog(app_id_.c_str(), LOG_PID | LOG_NDELAY, LOG_USER);
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
