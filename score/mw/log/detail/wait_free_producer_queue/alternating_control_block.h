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

#ifndef SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_CONTROL_BLOCK_H
#define SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_CONTROL_BLOCK_H

#include "score/mw/log/detail/wait_free_producer_queue/linear_control_block.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

// ----- COMMON_ARGUMENTATION ----
// Maintaining compatibility and avoiding performance overhead outweighs POD Type (class) based design. The Struct
// is ONLY used internally under the namespace detail and ONLY for data_router sub-dir, it is NOT exposed publicly;
// this is additionally guaranteed by the build system(bazel) visibility. Moreover, the Type is simple and does not
// require invariance (interface OR custom behavior) as per the design.
// -------------------------------

struct AlternatingControlBlock
{
    // COMMON_ARGUMENTATION
    // coverity[autosar_cpp14_m11_0_1_violation]
    LinearControlBlock control_block_even{};
    // COMMON_ARGUMENTATION
    // coverity[autosar_cpp14_m11_0_1_violation]
    LinearControlBlock control_block_odd{};
    // switch count is used to select buffer active for writing. Odd value selects control_block_odd for writing, even
    // value points control_block_even for writing.
    // COMMON_ARGUMENTATION
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::atomic<std::uint32_t> switch_count_points_active_for_writing{0UL};
};

/// \brief Initializes AlternatingControlBlock to set reader and writer side of the buffers making a 0-index buffer
/// reserved for reader and 1-indexed buffer available for writer. Switch counter is set to 1 pointing to writer buffer.
AlternatingControlBlock& InitializeAlternatingControlBlock(AlternatingControlBlock& alternating_control_block);

enum class AlternatingControlBlockSelectId : std::uint8_t
{
    kBlockEven,
    kBlockOdd,
};

//  Template function is used to resolve return type as const or non-const depending on iput arguments type.
template <typename T,
          typename = std::enable_if_t<std::is_same<std::remove_cv_t<T>, AlternatingControlBlock>::value, bool>>
auto& SelectLinearControlBlockReference(AlternatingControlBlockSelectId block_id, T& control)
{
    if (block_id == AlternatingControlBlockSelectId::kBlockEven)
    {
        return control.control_block_even;
    }
    else
    {
        return control.control_block_odd;
    }
}

AlternatingControlBlockSelectId GetOppositeLinearControlBlock(const AlternatingControlBlockSelectId id);
AlternatingControlBlockSelectId SelectLinearControlBlockId(std::uint32_t count);

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif
