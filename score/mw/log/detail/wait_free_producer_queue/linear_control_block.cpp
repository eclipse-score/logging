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

#include "score/mw/log/detail/wait_free_producer_queue/linear_control_block.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \returns true if number_of_bytes fits in control_block at the offset.
bool DoBytesFitInRemainingCapacity(const score::cpp::span<Byte>& buffer,
                                   const Length offset,
                                   const Length number_of_bytes) noexcept
{
    const Length buffer_size = GetDataSizeAsLength(buffer);

    if (offset > buffer_size)
    {
        return false;
    }

    const auto remaining_number_of_bytes_at_offset = buffer_size - offset;

    if (number_of_bytes > remaining_number_of_bytes_at_offset)
    {
        return false;
    }

    return true;
}

Length GetDataSizeAsLength(const score::cpp::span<Byte>& data) noexcept
{
    // Cast from non-negative signed size type to unsigned length is safe.
    static_assert(sizeof(Length) >= sizeof(SpanLength), "Length shall contain positive values of span size type");
    return static_cast<Length>(data.size());
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
