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

#include "score/mw/log/detail/wait_free_producer_queue/alternating_reader_proxy.h"

#include <thread>

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
auto GetSplitBlocks(AlternatingControlBlockSelectId block_id_active_for_writing,
                    AlternatingControlBlock& alternating_control_block)
{
    return std::tuple{ReusedCleanupBlockReference{SelectLinearControlBlockReference(
                          GetOppositeLinearControlBlock(block_id_active_for_writing), alternating_control_block)},
                      TerminatingBlockReference{
                          SelectLinearControlBlockReference(block_id_active_for_writing, alternating_control_block)}};
}
}  // namespace

AlternatingReaderProxy::AlternatingReaderProxy(AlternatingControlBlock& dcb) noexcept
    : alternating_control_block_(dcb),
      previous_logging_ipc_counter_value_{dcb.switch_count_points_active_for_writing.load()}
{
}

///  Assumption: The Switch method shall not be called from a concurrent contexts i.e. it supports single consumer.
std::uint32_t AlternatingReaderProxy::Switch() noexcept
{
    const auto switch_count_points_active_for_writing =
        alternating_control_block_.switch_count_points_active_for_writing.load();

    auto block_id_active_for_writing = SelectLinearControlBlockId(switch_count_points_active_for_writing);

    auto [restarting_control_block, terminating_control_block_intermediate] =
        GetSplitBlocks(block_id_active_for_writing, alternating_control_block_);

    std::ignore = terminating_control_block_intermediate;

    //  Reset counters for writing new data into restarting block.
    const auto acquired_index = restarting_control_block.GetReusedCleanupBlock().acquired_index.exchange(0);
    const auto written_index = restarting_control_block.GetReusedCleanupBlock().written_index.exchange(0);
    std::ignore = acquired_index;
    std::ignore = written_index;

    // Switch the active buffer for future writers.
    const auto save_switch_count = alternating_control_block_.switch_count_points_active_for_writing.fetch_add(1UL);

    std::atomic_thread_fence(std::memory_order_release);

    // Writer switch may be incomplete. It is not yet safe to read the data in the buffer.
    // It is left as reader responsibility to check if writers released buffer.

    previous_logging_ipc_counter_value_ = save_switch_count + 1;
    return save_switch_count;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
