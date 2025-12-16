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

#include "score/mw/log/detail/wait_free_producer_queue/wait_free_linear_writer.h"

#include <algorithm>

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

/// \brief We already incremented the atomic counter, but noted afterwards that
/// our payload does not fit anymore. In this case we attempt to write at least
/// the length to signal the reader that this was a failed acquisition. If even
/// the length does not fit anymore it is obvious to the reader that it was a
/// failed acquisition.
void TerminateBuffer(LinearControlBlock& control_block, const Length offset, const Length length) noexcept
{
    // Check if at least length fits in the remaining space.
    if (DoBytesFitInRemainingCapacity(control_block.data, offset, GetLengthOffsetBytes()))
    {
        const auto length_span = control_block.data.subspan(static_cast<SpanLength>(offset),
                                                            static_cast<SpanLength>(GetLengthOffsetBytes()));
        // clang-format off
        // we need to convert void pointer to bytes for serialization purposes, no out of bounds there
        // coverity[autosar_cpp14_m5_2_8_violation]
        const auto src_span = score::cpp::span<const Byte>{static_cast<const Byte*>(static_cast<const void*>(&length)), sizeof(length)};
        // clang-format on
        std::ignore = std::copy_n(src_span.begin(), GetLengthOffsetBytes(), length_span.begin());
    }

    // We must increment the written_index even for failed acquisition cases to ensure the condition
    // written_index == acquired_index that allows the reader to determine if all writers have finished
    // accessing the buffer.
    // The reader shall be able to detect failed acquisition by bounds checking of the buffer size.

    // The summation will not exceed uint64 because:
    // - GetLengthOffsetBytes() is returning sizeof(uint64) which equal to 8.
    // - length is already validated in CheckAndGetAcquireOffset by ensuring it does not exceed GetMaxAcquireLengthBytes
    // which within uint64.
    // coverity[autosar_cpp14_a4_7_1_violation]
    std::ignore = control_block.written_index.fetch_add(length + GetLengthOffsetBytes());
}

score::cpp::optional<Length> CheckAndGetAcquireOffset(LinearControlBlock& control_block,
                                               const Length length,
                                               const Length writer_concurrency,
                                               PreAcquireHook& pre_acquire_hook,
                                               WaitFreeLinearWriter& writer) noexcept
{
    if (writer_concurrency > GetMaxNumberOfConcurrentWriters())
    {
        // Too many writers.
        return {};
    }

    if (length > GetMaxAcquireLengthBytes())
    {
        // Not safe to increase
        return {};
    }

    const auto total_acquired_length = length + GetLengthOffsetBytes();

    // Check if it makes sense to increment the atomic counter, or if we are already full.
    const auto old_offset = control_block.acquired_index.load();

    // Avoid that the acquired_index could overflow.
    if (old_offset >= GetMaxLinearBufferCapacityBytes())
    {
        // Not safe to increase
        return {};
    }

    if (DoBytesFitInRemainingCapacity(control_block.data, old_offset, total_acquired_length) == false)
    {
        // Already not enough space left.
        return {};
    }

    pre_acquire_hook(writer);

    // We probably have enough space, attempt to acquire space on the buffer.
    const auto offset = control_block.acquired_index.fetch_add(total_acquired_length);

    if (DoBytesFitInRemainingCapacity(control_block.data, offset, total_acquired_length) == false)
    {
        // Someone was faster, buffer is already full meanwhile.
        TerminateBuffer(control_block, offset, length);
        return {};
    }

    return offset;
}

}  // namespace

WaitFreeLinearWriter::WaitFreeLinearWriter(LinearControlBlock& cb, PreAcquireHook pre_acquire_hook) noexcept
    : control_block_(cb), pre_acquire_hook_{std::move(pre_acquire_hook)}
{
}

score::cpp::optional<AcquiredData> WaitFreeLinearWriter::Acquire(const Length length) noexcept
{
    ++control_block_.number_of_writers;
    const auto writer_concurrency = control_block_.number_of_writers.load();

    const auto offset_result =
        CheckAndGetAcquireOffset(control_block_, length, writer_concurrency, pre_acquire_hook_, *this);

    if (offset_result.has_value() == false)
    {
        control_block_.number_of_writers--;
        return {};
    }

    const Length offset = offset_result.value();

    // Copy length to the beginning of the acquired range.
    const auto length_span =
        control_block_.data.subspan(static_cast<SpanLength>(offset), static_cast<SpanLength>(GetLengthOffsetBytes()));
    // clang-format off
    // we need to convert void pointer to bytes for serialization purposes, no out of bounds there
    // coverity[autosar_cpp14_m5_2_8_violation]
    const auto src_span = score::cpp::span<const Byte>{static_cast<const Byte*>(static_cast<const void*>(&length)), sizeof(length)};
    // clang-format on
    std::ignore = std::copy_n(src_span.begin(), GetLengthOffsetBytes(), length_span.begin());

    // static_casts are safe by bounds checking in CheckAndGetAcquireOffset().
    const auto payload_offset = offset + GetLengthOffsetBytes();
    const auto payload_span =
        control_block_.data.subspan(static_cast<SpanLength>(payload_offset), static_cast<SpanLength>(length));
    return AcquiredData{payload_span};
}

void WaitFreeLinearWriter::Release(const AcquiredData& acquired_data) noexcept
{
    // Fence is needed to ensure non atomic data is seen as written
    // before the index is updated.
    std::atomic_thread_fence(std::memory_order_release);

    // Suppress "AUTOSAR C++14 A4-7-1" rule finding. This rule states: "An integer expression shall
    // not lead to data loss.".
    // Rationale: The arithmetic to compute the aligned size is performed entirely with std::size_t values,
    // ensuring that no truncation occurs.
    std::ignore =
        // coverity[autosar_cpp14_a4_7_1_violation]
        control_block_.written_index.fetch_add(static_cast<size_t>(acquired_data.data.size()) + GetLengthOffsetBytes());

    control_block_.number_of_writers--;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
