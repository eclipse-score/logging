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

#include "score/mw/log/detail/wait_free_producer_queue/linear_reader.h"

#include <score/utility.hpp>

#include <algorithm>
#include <cstring>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

LinearReader::LinearReader(const score::cpp::span<Byte>& data) noexcept : data_{data}, read_index_{} {}

std::optional<score::cpp::span<Byte>> LinearReader::Read() noexcept
{
    const auto offset = read_index_;

    if (DoBytesFitInRemainingCapacity(data_, offset, GetLengthOffsetBytes()) == false)
    {
        return {};
    }

    // Cast is safe by bounds check above.
    const auto offset_casted = static_cast<SpanLength>(offset);
    const auto length_span = data_.subspan(offset_casted, static_cast<SpanLength>(GetLengthOffsetBytes()));
    Length length{};
    // We need to convert void pointer to bytes for serialization purposes, no out of bounds there.
    // coverity[autosar_cpp14_m5_2_8_violation]
    const auto dst_span = score::cpp::span<Byte>{static_cast<Byte*>(static_cast<void*>(&length)), sizeof(length)};
    std::ignore = std::copy_n(length_span.begin(), GetLengthOffsetBytes(), dst_span.begin());

    if (length > GetMaxAcquireLengthBytes())
    {
        // Unexpected high length value, drop all remaining data.
        read_index_ = GetDataSizeAsLength(data_);
        return {};
    }

    read_index_ += length + GetLengthOffsetBytes();

    if (DoBytesFitInRemainingCapacity(data_, offset, GetLengthOffsetBytes() + length) == false)
    {
        return {};
    }

    // Calculate the offset where the actual user payload lies behind the length prefix.
    const Length payload_offset = offset + GetLengthOffsetBytes();

    // static_casts are safe due to the bounds check above.
    return data_.subspan(static_cast<SpanLength>(payload_offset), static_cast<SpanLength>(length));
}

LinearReader CreateLinearReaderFromControlBlock(const LinearControlBlock& control_block) noexcept
{
    return CreateLinearReaderFromDataAndLength(control_block.data, control_block.written_index.load());
}

Length LinearReader::GetSizeOfWholeDataBuffer() const noexcept
{
    return GetDataSizeAsLength(data_);
}

LinearReader CreateLinearReaderFromDataAndLength(const score::cpp::span<Byte>& data,
                                                 const Length number_of_bytes_written) noexcept
{
    const Length data_length = GetDataSizeAsLength(data);
    const auto number_of_bytes_to_read = std::min(number_of_bytes_written, data_length);

    // static_cast is safe due to min limitation about.
    const auto number_of_bytes_to_read_casted = static_cast<SpanLength>(number_of_bytes_to_read);

    const auto data_cropped = data.subspan(0, number_of_bytes_to_read_casted);
    return LinearReader(data_cropped);
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
