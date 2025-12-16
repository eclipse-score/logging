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

#include "score/mw/log/detail/wait_free_producer_queue/wait_free_alternating_writer.h"

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

void ReleaseBlock(AlternatingControlBlockSelectId block_id, AlternatingControlBlock& alternating_control_block)
{
    auto& block_ref = SelectLinearControlBlockReference(block_id, alternating_control_block);
    std::ignore = block_ref.number_of_writers.fetch_sub(1U);
}

//  For a given loaded switch counter value, the AcquireBlock increases the number_of_writers value
std::optional<AlternatingControlBlockSelectId> AcquireBlock(std::uint32_t loaded_switch_counter_value,
                                                            AlternatingControlBlock& alternating_control_block)
{
    const auto candidate_block_id_active_for_writing = SelectLinearControlBlockId(loaded_switch_counter_value);
    auto& writing_block_reference =
        SelectLinearControlBlockReference(candidate_block_id_active_for_writing, alternating_control_block);

    //  Mark the try to acquire given block. Remember to release it when lost arbitration.
    //  This operation shall block reader to progress
    std::ignore = writing_block_reference.number_of_writers.fetch_add(1U, std::memory_order_acquire);

    const auto check_atomic_transition_valid_counter =
        alternating_control_block.switch_count_points_active_for_writing.load();

    // This function increments the acquired buffer ID. When the value reaches its maximum representable limit,
    // it wraps around to zero due to the well-defined unsigned integer overflow behavior.
    // This behavior is intentional and designed to ensure seamless buffer ID cycling.
    // coverity[autosar_cpp14_a4_7_1_violation]
    const auto second_atomic_transition_counter_value = loaded_switch_counter_value + 1U;

    if (check_atomic_transition_valid_counter == loaded_switch_counter_value)
    {
        //  switch has not happened and the writer was able to acquire block. The block can be returned to the user.
        return candidate_block_id_active_for_writing;
    }
    // clang-format off
    else if (check_atomic_transition_valid_counter == second_atomic_transition_counter_value) // LCOV_EXCL_BR_LINE: It
                                                      // is difficult (or may impossible) to achieve
                                                      // the TRUE of this condition because it requires to
                                                      // increment the switch_count_points_active_for_writing only once
                                                      // after the last occurring load at Acquire method.
    // clang-format on
    //  switch was incremented by one
    {
        // LCOV_EXCL_START
        //  The switch happened before we were able to increment 'number_of_writers' and initial candidate block was
        //  reserved by the reader i.e. we lost arbitrage. We are currently blocking an increment for future, but only
        //  second after the next one which still leaves some room for error even after considering the assumption that
        //  reader should not send another Read Acquire Request until writer releases old buffer. Thanks to checking
        //  this condition, we were able to move the problem until second switch happens i.e. concurrent operations are
        //  safe within the scope of one Switch operation. To resolve the issue completely the atomic operation shall be
        //  changed in a way that reading the state would block reader from progressing until the operation is resolved.
        //  It can be done by first acquiring blindly both blocks, resolving the selection based on block pointer and
        //  reader reservation flags and releasing not selected block only after the selection process is finished.
        const auto concurrently_changed_block_id_active_for_writing =
            GetOppositeLinearControlBlock(candidate_block_id_active_for_writing);
        auto& concurrently_changed_writing_block_reference = SelectLinearControlBlockReference(
            concurrently_changed_block_id_active_for_writing, alternating_control_block);

        std::ignore =
            concurrently_changed_writing_block_reference.number_of_writers.fetch_add(1U, std::memory_order_acquire);

        const auto second_check_atomic_transition_valid_counter =
            alternating_control_block.switch_count_points_active_for_writing.load();

        //  Release a candidate after acquiring the other block to block possible progress of the reader.
        std::ignore = writing_block_reference.number_of_writers.fetch_sub(1U, std::memory_order_release);

        if (second_check_atomic_transition_valid_counter != second_atomic_transition_counter_value)
        {
            std::ignore =
                concurrently_changed_writing_block_reference.number_of_writers.fetch_sub(1U, std::memory_order_release);
            return std::nullopt;
        }

        return concurrently_changed_block_id_active_for_writing;
        // LCOV_EXCL_STOP
    }

    //  Switch has happened more than once. This shall not happen as current writer still holds one block which was
    //  acquired by incrementing number_of_writers member. Abort performing functions as it is fatal error.
    std::ignore = writing_block_reference.number_of_writers.fetch_sub(1U, std::memory_order_release);
    return std::nullopt;
}

}  //  anonymous namespace

WaitFreeAlternatingWriter::WaitFreeAlternatingWriter(AlternatingControlBlock& control_block) noexcept
    : alternating_control_block_(control_block),
      wait_free_writing_even_(alternating_control_block_.control_block_even),
      wait_free_writing_odd_(alternating_control_block_.control_block_odd)
{
}

std::optional<AlternatingAcquiredData> WaitFreeAlternatingWriter::AcquireLinearDataOnAcquiredBlock(
    AlternatingControlBlockSelectId block_id_active_for_writing_value,
    Length length) noexcept
{
    if (block_id_active_for_writing_value == AlternatingControlBlockSelectId::kBlockEven)
    {
        const auto acquired_linear_data = wait_free_writing_even_.Acquire(length);
        if (acquired_linear_data.has_value())
        {
            return AlternatingAcquiredData{acquired_linear_data->data, block_id_active_for_writing_value};
        }
    }
    else
    {
        const auto acquired_linear_data = wait_free_writing_odd_.Acquire(length);
        if (acquired_linear_data.has_value())
        {
            return AlternatingAcquiredData{acquired_linear_data->data, block_id_active_for_writing_value};
        }
    }
    return std::nullopt;
}

//  Causes the increment of 'number_of_writers' in selected block. Remember to decrement the value when releasing.
std::optional<AlternatingAcquiredData> WaitFreeAlternatingWriter::Acquire(const Length length)
{
    const auto switch_count_points_active_for_writing =
        alternating_control_block_.switch_count_points_active_for_writing.load();

    const auto block_id_active_for_writing =
        AcquireBlock(switch_count_points_active_for_writing, alternating_control_block_);

    if (!block_id_active_for_writing.has_value())
    {
        return std::nullopt;
    }
    const auto block_id_active_for_writing_value = block_id_active_for_writing.value();

    const auto acquired_data = AcquireLinearDataOnAcquiredBlock(block_id_active_for_writing_value, length);

    //  Release Block as part of finishing the selection operation, block is still acquired by operation called on
    //  WaitFreeLinearWriter
    ReleaseBlock(block_id_active_for_writing_value, alternating_control_block_);

    return acquired_data;
}

void WaitFreeAlternatingWriter::Release(const AlternatingAcquiredData& acquired_data) noexcept
{
    if (acquired_data.control_block_id == AlternatingControlBlockSelectId::kBlockEven)
    {
        wait_free_writing_even_.Release(AcquiredData{acquired_data.data});
    }
    else
    {
        wait_free_writing_odd_.Release(AcquiredData{acquired_data.data});
    }
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
