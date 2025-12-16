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

#ifndef SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_WRITER_H
#define SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_WRITER_H

#include "score/mw/log/detail/wait_free_producer_queue/alternating_control_block.h"
#include "score/mw/log/detail/wait_free_producer_queue/wait_free_linear_writer.h"

#include <optional>

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
// ------------------------------

struct AlternatingAcquiredData
{
    // COMMON_ARGUMENTATION
    // coverity[autosar_cpp14_m11_0_1_violation]
    score::cpp::span<Byte> data;
    // NOLINTBEGIN(cppcoreguidelines-pro-type-member-init) COMMON_ARGUMENTATION provided in the struct header.
    // COMMON_ARGUMENTATION
    // coverity[autosar_cpp14_m11_0_1_violation]
    AlternatingControlBlockSelectId control_block_id{AlternatingControlBlockSelectId::kBlockEven};
    // NOLINTEND(cppcoreguidelines-pro-type-member-init) COMMON_ARGUMENTATION provided in the struct header.
};

/// \brief Wait-free writing to two alternating linear buffers.
/// Thread-safe for multiple writers.
class WaitFreeAlternatingWriter
{
  public:
    explicit WaitFreeAlternatingWriter(AlternatingControlBlock& control_block) noexcept;

    /// \brief Try to acquire the length for writing.
    /// Returns empty if there is not enough space available.
    std::optional<AlternatingAcquiredData> Acquire(const Length length);

    /// \brief Release the acquired data.
    void Release(const AlternatingAcquiredData& acquired_data) noexcept;

  private:
    std::optional<AlternatingAcquiredData> AcquireLinearDataOnAcquiredBlock(
        AlternatingControlBlockSelectId block_id_active_for_writing_value,
        Length length) noexcept;

    AlternatingControlBlock& alternating_control_block_;
    WaitFreeLinearWriter wait_free_writing_even_;
    WaitFreeLinearWriter wait_free_writing_odd_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif
