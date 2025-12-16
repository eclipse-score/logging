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

#include "score/mw/log/detail/wait_free_producer_queue/alternating_control_block.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

AlternatingControlBlockSelectId SelectLinearControlBlockId(std::uint32_t count)
{
    if (count % 2U == 0U)
    {
        return AlternatingControlBlockSelectId::kBlockEven;
    }
    else
    {
        return AlternatingControlBlockSelectId::kBlockOdd;
    }
}

AlternatingControlBlockSelectId GetOppositeLinearControlBlock(const AlternatingControlBlockSelectId id)
{
    AlternatingControlBlockSelectId return_value{AlternatingControlBlockSelectId::kBlockEven};
    switch (id)
    {
        case AlternatingControlBlockSelectId::kBlockOdd:
            return_value = AlternatingControlBlockSelectId::kBlockEven;
            break;
        case AlternatingControlBlockSelectId::kBlockEven:
            return_value = AlternatingControlBlockSelectId::kBlockOdd;
            break;
        default:
            break;
    }
    return return_value;
}

AlternatingControlBlock& InitializeAlternatingControlBlock(AlternatingControlBlock& alternating_control_block)
{
    alternating_control_block.switch_count_points_active_for_writing = 1UL;
    return alternating_control_block;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
