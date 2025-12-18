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

#include "score/mw/log/detail/data_router/data_router_recorder.h"

#include "score/mw/log/detail/common/dlt_format.h"
#include "score/mw/log/detail/data_router/data_router_backend.h"
#include "score/mw/log/detail/dlt_argument_counter.h"

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

constexpr auto kStatisticsReportInterval = std::chrono::seconds{5};

void CleanLogRecord(LogRecord& logRecord) noexcept
{
    auto& log_entry = logRecord.getLogEntry();
    log_entry.num_of_args = 0U;
    log_entry.payload.clear();
}

void SetContext(LogRecord& logRecord, const std::string_view context_id) noexcept
{
    auto& log_entry = logRecord.getLogEntry();
    log_entry.ctx_id = LoggingIdentifier{context_id};
}

void SetLogLevel(LogRecord& logRecord, const LogLevel level) noexcept
{
    static_assert(std::is_same<std::underlying_type<LogLevel>::type, std::uint8_t>::value,
                  "LogLevel is not of expected type. Static cast will be invalid.");
    logRecord.getLogEntry().log_level = level;
}

}  // namespace

DataRouterRecorder::DataRouterRecorder(std::unique_ptr<Backend>&& backend, const Configuration& config) noexcept
    : Recorder{},
      backend_(std::move(backend)),
      config_{config},
      statistics_reporter_{*this, kStatisticsReportInterval, config.GetNumberOfSlots(), config.GetSlotSizeInBytes()}
{
}

score::cpp::optional<SlotHandle> DataRouterRecorder::StartRecord(const std::string_view context_id,
                                                          const LogLevel log_level) noexcept
{
    statistics_reporter_.Update(std::chrono::steady_clock::now());

    if (IsLogEnabled(log_level, context_id) == false)
    {
        return {};
    }

    const auto& slot = backend_->ReserveSlot();

    if (slot.has_value())
    {
        auto&& logRecord = backend_->GetLogRecord(*slot);
        CleanLogRecord(logRecord);
        SetApplicationId(logRecord);
        SetContext(logRecord, context_id);
        SetLogLevel(logRecord, log_level);
    }
    else
    {
        statistics_reporter_.IncrementNoSlotAvailable();
    }

    return slot;
}

void DataRouterRecorder::StopRecord(const SlotHandle& slot) noexcept
{
    backend_->FlushSlot(slot);
}

template <typename T>
void DataRouterRecorder::LogData(const SlotHandle& slot, const T data) noexcept
{
    auto& logRecord = backend_->GetLogRecord(slot);
    DltArgumentCounter counter{logRecord.getLogEntry().num_of_args};
    std::ignore = counter.TryAddArgument([data, &logRecord, this]() noexcept {
        const auto result = DLTFormat::Log(logRecord.getVerbosePayload(), data);
        if (result == AddArgumentResult::NotAdded)
        {
            this->statistics_reporter_.IncrementMessageTooLong();
        }
        return result;
    });
}

void DataRouterRecorder::Log(const SlotHandle& slot, const bool data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::uint8_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::int8_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::uint16_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::int16_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::uint32_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::int32_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::uint64_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::int64_t data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const float data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const double data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const std::string_view data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogHex8 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogHex16 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogHex32 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogHex64 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogBin8 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogBin16 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogBin32 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogBin64 data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogRawBuffer data) noexcept
{
    LogData(slot, data);
}

void DataRouterRecorder::Log(const SlotHandle& slot, const LogSlog2Message data) noexcept
{
    LogData(slot, data.GetMessage());
}

void DataRouterRecorder::SetApplicationId(LogRecord& logRecord) noexcept
{
    auto& log_entry = logRecord.getLogEntry();
    const auto app_id = config_.GetAppId();
    log_entry.app_id = LoggingIdentifier{app_id};
}

bool DataRouterRecorder::IsLogEnabled(const LogLevel& log_level, const std::string_view context) const noexcept
{
    return config_.IsLogLevelEnabled(log_level, context);
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
