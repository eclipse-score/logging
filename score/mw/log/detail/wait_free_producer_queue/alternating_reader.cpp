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

#include "score/mw/log/detail/wait_free_producer_queue/alternating_reader.h"

#include <thread>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

AlternatingReadOnlyReader::AlternatingReadOnlyReader(const AlternatingControlBlock& dcb,
                                                     const score::cpp::span<Byte> buffer_even,
                                                     const score::cpp::span<Byte> buffer_odd) noexcept
    : alternating_control_block_(dcb), reader_({}), buffer_even_{buffer_even}, buffer_odd_{buffer_odd}
{
}

LinearReader AlternatingReadOnlyReader::CreateLinearReader(const std::uint32_t block_id_count) noexcept
{
    auto block_id = SelectLinearControlBlockId(block_id_count);
    auto& block = SelectLinearControlBlockReference(block_id, alternating_control_block_);

    const auto written_bytes = block.written_index.load();

    auto& buffer = block_id == AlternatingControlBlockSelectId::kBlockEven ? buffer_even_ : buffer_odd_;
    return CreateLinearReaderFromDataAndLength(buffer, written_bytes);
}

bool AlternatingReadOnlyReader::IsBlockReleasedByWriters(const std::uint32_t block_id_count) const noexcept
{
    auto block_id = SelectLinearControlBlockId(block_id_count);
    auto& block = SelectLinearControlBlockReference(block_id, alternating_control_block_);

    const bool result = (block.number_of_writers.load() == static_cast<Length>(0)) &&
                        (block.written_index.load() == block.acquired_index.load());
    if (result)
    {
        std::atomic_thread_fence(std::memory_order_acquire);
    }
    return result;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
