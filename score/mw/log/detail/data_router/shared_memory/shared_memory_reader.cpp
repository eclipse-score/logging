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

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"

#include <iostream>
#include <type_traits>

namespace score
{

namespace mw
{

namespace log
{

namespace detail
{

namespace
{
Length ReadLinearBuffer(LinearReader& reader,
                        const TypeRegistrationCallback& type_registration_callback,
                        const NewRecordCallback& new_message_callback) noexcept
{
    const Length length{reader.GetSizeOfWholeDataBuffer()};
    while (true)
    {
        const auto read_result = reader.Read();
        if (read_result.has_value() == false)
        {
            break;
        }

        if (GetDataSizeAsLength(read_result.value()) < sizeof(BufferEntryHeader))
        {
            /*
                Deviation from Rule M6-6-3:
                - The continue statement shall only be used within a well-formed for loop.
                Justification:
                - Used in While loop to check for Invalid payload.
            */
            // coverity[autosar_cpp14_m6_6_3_violation]
            continue;
        }

        // Extract header
        BufferEntryHeader header{};
        /*
            Deviation from Rule M5-2-8:
            - An object with integer type or pointer to void type shall not be converted
                to an object with pointer type.
            Justification:
            - We need to convert header to bytes (raw data) to read from
              read_result into object header.
        */
        // coverity[autosar_cpp14_m5_2_8_violation]
        auto header_destination_span = score::cpp::span<Byte>{static_cast<Byte*>(static_cast<void*>(&header)), sizeof(header)};
        const auto header_source_span = read_result->subspan(0, sizeof(BufferEntryHeader));
        std::ignore =
            std::copy(header_source_span.cbegin(), header_source_span.cend(), header_destination_span.begin());
        const auto payload_span = read_result->subspan(sizeof(BufferEntryHeader));

        if (header.type_identifier == score::mw::log::detail::GetRegisterTypeToken())
        {
            TypeRegistration type_registration{};
            if (GetDataSizeAsLength(payload_span) < sizeof(type_registration.type_id))
            {
                /*
                    Deviation from Rule M6-6-3:
                    - The continue statement shall only be used within a well-formed for loop.
                    Justification:
                    - Used in While loop to check for invalid size of registered type.
                */
                // coverity[autosar_cpp14_m6_6_3_violation]
                continue;
            }
            static_assert(std::is_trivially_copyable_v<decltype(type_registration.type_id)> == true);
            const auto kTypeIdSize = sizeof(type_registration.type_id);
            const auto type_id_source = payload_span.subspan(0, kTypeIdSize);
            auto type_id_destination =
                /*
                    Deviation from Rule M5-2-8:
                    - An object with integer type or pointer to void type shall not be converted
                        to an object with pointer type.
                    Justification:
                    - We need to convert type_registration.type_id to bytes (raw data) to read from
                      payload_span into object type_registration.type_id.
                */
                // coverity[autosar_cpp14_m5_2_8_violation]
                score::cpp::span<Byte>{static_cast<Byte*>(static_cast<void*>(&type_registration.type_id)), kTypeIdSize};
            std::ignore = std::copy(type_id_source.cbegin(), type_id_source.cend(), type_id_destination.begin());

            // type_id is integer uint16_t and thus is within range of values
            // that std::ptrdiff_t which is long int can store
            static_assert(
                static_cast<std::ptrdiff_t>(std::numeric_limits<decltype(type_registration.type_id)>::max()) <=
                    std::numeric_limits<std::ptrdiff_t>::max(),
                "Incompatible type");
            const auto type_id_size = static_cast<std::ptrdiff_t>(sizeof(type_registration.type_id));
            type_registration.registration_data = payload_span.subspan(type_id_size);

            type_registration_callback(type_registration);
        }
        else
        {
            SharedMemoryRecord record{};
            record.header = header;
            record.payload = payload_span;
            new_message_callback(record);
        }
    }
    return length;
}

}  // namespace

SharedMemoryReader::SharedMemoryReader(const SharedData& shared_data,
                                       AlternatingReadOnlyReader alternating_read_only_reader,
                                       UnmapCallback unmap_callback) noexcept
    : shared_data_{shared_data},
      unmap_callback_{std::move(unmap_callback)},
      acquired_data_{},
      number_of_acquired_bytes_{},
      finished_reading_after_detach_{false},
      buffer_expected_to_read_next_{shared_data.control_block.switch_count_points_active_for_writing.load()},
      is_writer_detached_{false},
      alternating_read_only_reader_{std::move(alternating_read_only_reader)}
{
}

SharedMemoryReader::SharedMemoryReader(SharedMemoryReader&& other) noexcept
    : shared_data_{other.shared_data_},
      unmap_callback_{std::move(other.unmap_callback_)},
      acquired_data_{other.acquired_data_},
      number_of_acquired_bytes_{other.number_of_acquired_bytes_},
      finished_reading_after_detach_{other.finished_reading_after_detach_},
      buffer_expected_to_read_next_(other.buffer_expected_to_read_next_),
      is_writer_detached_{other.is_writer_detached_},
      alternating_read_only_reader_{std::move(other.alternating_read_only_reader_)}
{
}

SharedMemoryReader::~SharedMemoryReader()
{
    if (!unmap_callback_.empty())
    {
        unmap_callback_();
    }
}

std::optional<Length> SharedMemoryReader::Read(const TypeRegistrationCallback& type_registration_callback,
                                               const NewRecordCallback& new_message_callback) noexcept
{
    if (finished_reading_after_detach_)
    {
        return std::nullopt;
    }

    std::optional<Length> return_written_bytes{std::nullopt};

    if (linear_reader_.has_value())
    {
        return_written_bytes =
            ReadLinearBuffer(linear_reader_.value(), type_registration_callback, new_message_callback);
        linear_reader_.reset();
    }

    if (IsWriterDetached())
    {
        auto reader = alternating_read_only_reader_.CreateLinearReader(buffer_expected_to_read_next_);
        const auto written_bytes_detached = ReadLinearBuffer(reader, type_registration_callback, new_message_callback);
        if (return_written_bytes.has_value())
        {
            return_written_bytes = return_written_bytes.value() + written_bytes_detached;
        }
        else
        {
            return_written_bytes = written_bytes_detached;
        }

        finished_reading_after_detach_ = true;
    }

    return return_written_bytes;
}

std::optional<Length> SharedMemoryReader::ReadDetached(const TypeRegistrationCallback& type_registration_callback,
                                                       const NewRecordCallback& new_message_callback) noexcept
{
    this->DetachWriter();
    return this->Read(type_registration_callback, new_message_callback);
}

void SharedMemoryReader::DetachWriter() noexcept
{
    is_writer_detached_ = true;
}

Length SharedMemoryReader::GetNumberOfDropsWithBufferFull() const noexcept
{
    return shared_data_.number_of_drops_buffer_full.load();
}

Length SharedMemoryReader::GetSizeOfDropsWithBufferFull() const noexcept
{
    return shared_data_.size_of_drops_buffer_full.load();
}

Length SharedMemoryReader::GetNumberOfDropsWithInvalidSize() const noexcept
{
    return shared_data_.number_of_drops_invalid_size.load();
}

Length SharedMemoryReader::GetNumberOfDropsWithTypeRegistrationFailed() const noexcept
{
    return shared_data_.number_of_drops_type_registration_failed.load();
}

Length SharedMemoryReader::GetRingBufferSizeBytes() const noexcept
{
    return GetDataSizeAsLength(shared_data_.control_block.control_block_even.data) +
           GetDataSizeAsLength(shared_data_.control_block.control_block_odd.data);
}

bool SharedMemoryReader::IsWriterDetached() const noexcept
{
    return (is_writer_detached_ == true) || (shared_data_.writer_detached.load() == true);
}

bool SharedMemoryReader::IsBlockReleasedByWriters(const std::uint32_t block_count) noexcept
{
    return alternating_read_only_reader_.IsBlockReleasedByWriters(block_count);
}

std::optional<Length> SharedMemoryReader::NotifyAcquisitionSetReader(const ReadAcquireResult& acquire_result) noexcept
{
    if (not alternating_read_only_reader_.IsBlockReleasedByWriters(
            acquire_result.acquired_buffer))  //  , "Working on a block that was not released by writers");
    {
        std::cerr
            << "SharedMemoryReader: Writers did not release the buffers. Logging channel for this client maybe blocked"
            << std::endl;
        //  TODO: Add reporting to DR statistics module to print this information together with AppId.
        //  Handle it by stopping sending read acquire request. Blame it on logging client which has higher levels of
        //  safety qualification.
        return std::nullopt;
    }
    auto reader = alternating_read_only_reader_.CreateLinearReader(acquire_result.acquired_buffer);
    number_of_acquired_bytes_ = reader.GetSizeOfWholeDataBuffer();
    linear_reader_ = reader;

    buffer_expected_to_read_next_ = acquire_result.acquired_buffer + 1;
    return number_of_acquired_bytes_;
}

std::optional<Length> SharedMemoryReader::PeekNumberOfBytesAcquiredInBuffer(
    const std::uint32_t acquired_buffer_count_id) const noexcept
{
    auto block_id = SelectLinearControlBlockId(acquired_buffer_count_id);
    auto& block = SelectLinearControlBlockReference(block_id, shared_data_.control_block);

    return block.acquired_index.load();
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
