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

#ifndef SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_READER_H
#define SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_READER_H

#include "score/mw/log/detail/wait_free_producer_queue/alternating_control_block.h"
#include "score/mw/log/detail/wait_free_producer_queue/linear_reader.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class AlternatingReadOnlyReader
{
  public:
    explicit AlternatingReadOnlyReader(const AlternatingControlBlock& dcb,
                                       const score::cpp::span<Byte> buffer_even,
                                       const score::cpp::span<Byte> buffer_odd) noexcept;

    /// \brief Check if all the references to block pointed by block_id_count were dropped by the writers.
    /// Returns false if at least one buffer is still referenced by writer, true otherwise.
    bool IsBlockReleasedByWriters(const std::uint32_t block_id_count) const noexcept;
    /// \brief Creates LinearReader which is based on span of data pointing to memory directly within Shared-memory
    /// buffer. It must be synchronized by a user. Shall be called only after making sure that data is no longer being
    /// modified by writers.
    LinearReader CreateLinearReader(const std::uint32_t block_id_count) noexcept;

  private:
    const AlternatingControlBlock& alternating_control_block_;
    std::optional<LinearReader> reader_;
    const score::cpp::span<Byte> buffer_even_;
    const score::cpp::span<Byte> buffer_odd_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif
