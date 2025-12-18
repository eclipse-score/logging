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

#ifndef SCORE_MW_LOG_DETAIL_WAIT_FREE_PRODUCER_QUEUE_LINEAR_READER_H
#define SCORE_MW_LOG_DETAIL_WAIT_FREE_PRODUCER_QUEUE_LINEAR_READER_H

#include "score/mw/log/detail/wait_free_producer_queue/linear_control_block.h"

#include "score/span.hpp"

#include <optional>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief Reader for a linear buffer.
/// The Reader instance itself is not thread-safe and should only be used after
/// the last writer has finished.
class LinearReader
{
  public:
    explicit LinearReader(const score::cpp::span<Byte>& data) noexcept;

    /// \brief Try to read the next available data.
    /// Returns empty if the data is not available.
    std::optional<score::cpp::span<Byte>> Read() noexcept;
    /// \brief Get size of whole data span which means sum of length encoding headers and payload of each chunk
    Length GetSizeOfWholeDataBuffer() const noexcept;

  private:
    score::cpp::span<Byte> data_;
    Length read_index_;
};

LinearReader CreateLinearReaderFromControlBlock(const LinearControlBlock&) noexcept;
LinearReader CreateLinearReaderFromDataAndLength(const score::cpp::span<Byte>& data,
                                                 const Length number_of_bytes_written) noexcept;

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif
