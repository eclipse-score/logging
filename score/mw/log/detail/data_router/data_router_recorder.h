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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_RECORDER_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_RECORDER_H

#include "score/mw/log/recorder.h"

#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/detail/common/statistics_reporter.h"

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class Backend;
class LogRecord;

class DataRouterRecorder final : public Recorder
{
  public:
    DataRouterRecorder(std::unique_ptr<Backend>&&, const Configuration& config) noexcept;

    DataRouterRecorder(DataRouterRecorder&&) noexcept = delete;
    DataRouterRecorder(const DataRouterRecorder&) noexcept = delete;
    DataRouterRecorder& operator=(DataRouterRecorder&&) noexcept = delete;
    DataRouterRecorder& operator=(const DataRouterRecorder&) noexcept = delete;

    ~DataRouterRecorder() override = default;

    score::cpp::optional<SlotHandle> StartRecord(const std::string_view context_id,
                                          const LogLevel log_level) noexcept override;

    void StopRecord(const SlotHandle& slot) noexcept override;

    void Log(const SlotHandle&, const bool) noexcept override;
    void Log(const SlotHandle&, const std::uint8_t) noexcept override;
    void Log(const SlotHandle&, const std::int8_t) noexcept override;
    void Log(const SlotHandle&, const std::uint16_t) noexcept override;
    void Log(const SlotHandle&, const std::int16_t) noexcept override;
    void Log(const SlotHandle&, const std::uint32_t) noexcept override;
    void Log(const SlotHandle&, const std::int32_t) noexcept override;
    void Log(const SlotHandle&, const std::uint64_t) noexcept override;
    void Log(const SlotHandle&, const std::int64_t) noexcept override;
    void Log(const SlotHandle&, const float) noexcept override;
    void Log(const SlotHandle&, const double) noexcept override;
    void Log(const SlotHandle&, const std::string_view) noexcept override;

    void Log(const SlotHandle&, const LogHex8) noexcept override;
    void Log(const SlotHandle&, const LogHex16) noexcept override;
    void Log(const SlotHandle&, const LogHex32) noexcept override;
    void Log(const SlotHandle&, const LogHex64) noexcept override;

    void Log(const SlotHandle&, const LogBin8) noexcept override;
    void Log(const SlotHandle&, const LogBin16) noexcept override;
    void Log(const SlotHandle&, const LogBin32) noexcept override;
    void Log(const SlotHandle&, const LogBin64) noexcept override;

    void Log(const SlotHandle&, const LogRawBuffer) noexcept override;

    void Log(const SlotHandle&, const LogSlog2Message) noexcept override;

    bool IsLogEnabled(const LogLevel& log_level, const std::string_view context) const noexcept override;

  private:
    template <typename T>
    void LogData(const SlotHandle&, const T data) noexcept;

    void SetApplicationId(LogRecord&) noexcept;

    std::unique_ptr<Backend> backend_;
    Configuration config_;
    StatisticsReporter statistics_reporter_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_RECORDER_H
