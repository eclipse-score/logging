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

#ifndef BMW_MW_LOG_WAIT_FREE_SHARED_MEMORY_COMMON
#define BMW_MW_LOG_WAIT_FREE_SHARED_MEMORY_COMMON

#include "score/os/utils/high_resolution_steady_clock.h"
#include "score/mw/log/detail/wait_free_producer_queue/alternating_control_block.h"

#include <score/callback.hpp>

#include <atomic>
#include <limits>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
struct SharedData
{
    /*
        Maintaining compatibility and avoiding performance overhead outweighs POD Type (class) based design for this
       particular struct. The Type is simple and does not require invariance (interface OR custom behavior) as per the
       design. Moreover the type is ONLY used internally under the namespace detail and NOT exposed publicly; this is
       additionally guaranteed by the build system(bazel) visibility
    */
    // coverity[autosar_cpp14_m11_0_1_violation]
    AlternatingControlBlock control_block{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    Length linear_buffer_1_offset{};  // Allows the reader to determine the buffer address in shared-memory.
    // coverity[autosar_cpp14_m11_0_1_violation]
    Length linear_buffer_2_offset{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::atomic<Length> number_of_drops_buffer_full{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::atomic<Length> size_of_drops_buffer_full{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::atomic<Length> number_of_drops_invalid_size{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::atomic<Length> number_of_drops_type_registration_failed{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::atomic<bool> writer_detached{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    pid_t producer_pid{};  // Helps Datarouter to check if a sender pid matches the shared-memory file pid.
};

/// \brief This helper initialization method shall be only called once at the construction of the object in
/// shared-memory data. Called usually shortly after shared-memory object creation and mapping.
SharedData& InitializeSharedData(SharedData& shared_data);
using TimePoint = score::os::HighResolutionSteadyClock::time_point;
using TypeIdentifier = std::uint16_t;

/// \brief Callback that is injected to free shared-memory map.
// The default value for the second parameter (callback capacity) - 32UL set in amp library.
using UnmapCallback = score::cpp::callback<void(void), 128UL>;

/// \brief This is prepended in front of each entry in the ring buffer.
struct BufferEntryHeader
{
    /*
        Maintaining compatibility and avoiding performance overhead outweighs POD Type (class) based design for this
       particular struct. The Type is simple and does not require invariance (interface OR custom behavior) as per the
       design. Moreover the type is ONLY used internally under the namespace detail and NOT exposed publicly; this is
       additionally guaranteed by the build system(bazel) visibility
    */
    // coverity[autosar_cpp14_m11_0_1_violation]
    TimePoint time_stamp{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    TypeIdentifier type_identifier{};
};

struct ReadAcquireResult
{
    std::uint32_t acquired_buffer;
};

std::uint32_t GetExpectedNextAcquiredBlockId(const ReadAcquireResult& acquired) noexcept;

constexpr TypeIdentifier GetRegisterTypeToken()
{
    return std::numeric_limits<TypeIdentifier>::max();
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  //  BMW_MW_LOG_WAIT_FREE_SHARED_MEMORY_COMMON
