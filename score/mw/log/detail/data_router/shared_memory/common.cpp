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

#include "score/mw/log/detail/data_router/shared_memory/common.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

SharedData& InitializeSharedData(SharedData& shared_data)
{
    std::ignore = InitializeAlternatingControlBlock(shared_data.control_block);
    return shared_data;
}

std::uint32_t GetExpectedNextAcquiredBlockId(const ReadAcquireResult& acquired) noexcept
{
    // This function increments the acquired buffer ID. When the value reaches its maximum representable limit,
    // it wraps around to zero due to the well-defined unsigned integer overflow behavior.
    // This behavior is intentional and designed to ensure seamless buffer ID cycling.
    // coverity[autosar_cpp14_a4_7_1_violation]
    return acquired.acquired_buffer + 1u;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
