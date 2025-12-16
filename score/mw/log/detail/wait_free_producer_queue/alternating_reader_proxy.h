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

#ifndef SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_READER_PROXY_H
#define SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_READER_PROXY_H

#include "score/mw/log/detail/wait_free_producer_queue/alternating_control_block.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief Reader for two alternating linear buffers.
/// An instance of this class is not thread-safe and should only be used by a
/// single thread exclusively.
class AlternatingReaderProxy
{
  public:
    explicit AlternatingReaderProxy(AlternatingControlBlock& dcb) noexcept;

    /// \brief Alternate the buffers for reading and writing.
    /// Returns the value of counter before increment aka buffer acquried for reading.
    std::uint32_t Switch() noexcept;

  private:
    AlternatingControlBlock& alternating_control_block_;
    std::uint32_t previous_logging_ipc_counter_value_;
};

/// \brief Reader for two alternating linear buffers.
/// An instance of this class is not thread-safe and should only be used by a
/// single thread exclusively.
//  Wrapper structure used to enforce type checking:
template <typename T, typename = std::enable_if_t<std::is_same<std::remove_cv_t<T>, LinearControlBlock>::value, bool>>
class ReusedCleanupBlockReference
{
  public:
    explicit constexpr ReusedCleanupBlockReference(T& linear_control_block)
        : reused_cleanup_block_{linear_control_block}
    {
    }
    inline T& GetReusedCleanupBlock()
    {
        return reused_cleanup_block_;
    }

  private:
    T& reused_cleanup_block_;
};

//  Wrapper structure used to enforce type checking:
template <typename T, typename = std::enable_if_t<std::is_same<std::remove_cv_t<T>, LinearControlBlock>::value, bool>>
class TerminatingBlockReference
{
  public:
    explicit constexpr TerminatingBlockReference(T& linear_control_block) : terminating_block_{linear_control_block} {}
    inline T& GetTerminatingBlock()
    {
        return terminating_block_;
    }

  private:
    T& terminating_block_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  //  SCORE_MW_LOG_DETAIL_WAIT_FREE_ALTERNATING_READER_PROXY_H
