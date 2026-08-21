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
#ifndef SCORE_MW_LOG_DETAIL_SYSLOG_SYSLOG_BACKEND_H
#define SCORE_MW_LOG_DETAIL_SYSLOG_SYSLOG_BACKEND_H

#include "score/os/syslog.h"
#include "score/mw/log/detail/backend.h"
#include "score/mw/log/detail/circular_allocator.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class SyslogBackend final : public Backend
{
  public:
    explicit SyslogBackend(const std::size_t number_of_slots,
                           const LogRecord& initial_slot_value,
                           const std::string_view app_id,
                           score::cpp::pmr::unique_ptr<score::os::Syslog> syslog_instance) noexcept;

    score::cpp::optional<SlotHandle> ReserveSlot() noexcept override;
    void FlushSlot(const SlotHandle& slot) noexcept override;
    LogRecord& GetLogRecord(const SlotHandle& slot) noexcept override;

  private:
    void Init() noexcept;

    std::string app_id_;
    CircularAllocator<score::mw::log::detail::LogRecord> buffer_;
    score::cpp::pmr::unique_ptr<score::os::Syslog> syslog_instance_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_SYSLOG_SYSLOG_BACKEND_H
