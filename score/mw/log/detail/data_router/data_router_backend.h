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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_BACKEND_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_BACKEND_H

#include "score/mw/log/detail/backend.h"

#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/detail/circular_allocator.h"
#include "score/mw/log/detail/data_router/data_router_message_client.h"
#include "score/mw/log/detail/data_router/data_router_message_client_factory.h"
#include "score/mw/log/detail/log_record.h"

#include <cstdint>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class DataRouterBackend final : public Backend
{
  public:
    explicit DataRouterBackend(const std::size_t number_of_slots,
                               const LogRecord& initial_slot_value,
                               DatarouterMessageClientFactory& message_client_factory,
                               const Configuration& config,
                               WriterFactory writer_factory);

    score::cpp::optional<SlotHandle> ReserveSlot() noexcept override;
    void FlushSlot(const SlotHandle& slot) noexcept override;
    LogRecord& GetLogRecord(const SlotHandle& slot) noexcept override;

  private:
    CircularAllocator<score::mw::log::detail::LogRecord> buffer_;
    std::unique_ptr<DatarouterMessageClient> message_client_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_BACKEND_H
