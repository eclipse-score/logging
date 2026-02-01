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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_SHARED_MEMORY_WRITER_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_SHARED_MEMORY_WRITER_H

#include "score/mw/log/detail/data_router/shared_memory/common.h"
#include "score/mw/log/detail/wait_free_producer_queue/alternating_reader_proxy.h"
#include "score/mw/log/detail/wait_free_producer_queue/wait_free_alternating_writer.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief This class manages the writing of serialized data types on shared memory.
/// Before a type is traced with AllocAndWrite() it shall be registered with TryRegisterType().
class SharedMemoryWriter
{
  public:
    using size_type = score::cpp::span<Byte>::size_type;

    explicit SharedMemoryWriter(SharedData& shared_data, UnmapCallback unmap_callback) noexcept;

    //  Moving the object is not allowed during operation.
    //  It is allowed to be used only in initialization phase by a factory
    SharedMemoryWriter(SharedMemoryWriter&& other) noexcept;

    ~SharedMemoryWriter() noexcept;

    SharedMemoryWriter(const SharedMemoryWriter&) = delete;
    SharedMemoryWriter& operator=(const SharedMemoryWriter&) = delete;
    SharedMemoryWriter& operator=(SharedMemoryWriter&&) = delete;

    static constexpr Length GetMaxPayloadSize()
    {
        // max size of a dlt-v1 message excluding the header.
        constexpr std::uint64_t kValue = 65500UL;
        static_assert(GetMaxAcquireLengthBytes() >= kValue + sizeof(BufferEntryHeader),
                      "must not exceed limits of linear writer");
        return kValue;
    }

    /// \brief Allocates space on buffer and writes data into it.
    /// This method is thread-safe, lock-free and wait-free.
    template <typename WriteCallback>
    // Suppressing the "AUTOSAR C++14 A15-5-3" rule violation:
    // "The std::terminate() function shall not be called implicitly."
    // The function `AllocAndWrite` is marked `noexcept`, ensuring that it does not throw exceptions.
    // However, removing `noexcept` introduces additional Coverity findings, so refactoring is required.
    // coverity[autosar_cpp14_a15_5_3_violation]
    void AllocAndWrite(const TimePoint timestamp,
                       const TypeIdentifier type_identifier,
                       const Length payload_size,
                       WriteCallback write_callback) noexcept
    {
        if (payload_size > GetMaxPayloadSize())
        {
            shared_data_.number_of_drops_invalid_size++;
            return;
        }

        const Length total_size = payload_size + sizeof(BufferEntryHeader);
        const auto acquired_data = alternating_writer_.Acquire(total_size);

        if (acquired_data.has_value() == false)
        {
            shared_data_.number_of_drops_buffer_full++;
            shared_data_.size_of_drops_buffer_full += total_size;
            return;
        }

        // Write header
        const BufferEntryHeader header{
            timestamp,
            type_identifier,
        };
        const score::cpp::span<Byte> header_span = acquired_data.value().data.subspan(0, sizeof(BufferEntryHeader));
        // Suppress "AUTOSAR C++14 M5-2-8" rule. The rule declares:
        // An object with integer type or pointer to void type shall not be converted to an object with pointer type.
        // But we need to convert void pointer to bytes for serialization purposes, no out of bounds there
        // coverity[autosar_cpp14_m5_2_8_violation]
        const score::cpp::span<const Byte> header_source{static_cast<const char*>(static_cast<const void*>(&header)),
                                                  sizeof(header)};
        // coverity[autosar_cpp14_m5_0_16_violation:FALSE]
        std::ignore = std::copy(header_source.begin(), header_source.end(), header_span.begin());

        // Write payload
        const auto payload_span =
            acquired_data.value().data.subspan(sizeof(BufferEntryHeader), static_cast<size_type>(payload_size));
        // Suppressing the "AUTOSAR C++14 A15-4-2" rule violation:
        // This rule states: "If a function is declared as noexcept, noexcept(true), or noexcept(<true condition>),
        // then it shall not exit with an exception."
        // Removing `noexcept` would introduce new Coverity findings.
        // coverity[autosar_cpp14_a15_4_2_violation]
        write_callback(payload_span);

        alternating_writer_.Release(acquired_data.value());
    }

    /// \brief Allocates space on buffer and writes data into it.
    /// This method is thread-safe, lock-free and wait-free.
    template <typename WriteCallback>
    void AllocAndWrite(WriteCallback write_callback,
                       const TypeIdentifier type_identifier,
                       const Length payload_size) noexcept
    {
        AllocAndWrite(TimePoint::clock::now(), type_identifier, payload_size, write_callback);
    }

    /// \brief A type shall be registered successfully before tracing.
    /// The registration may fail if there is no space left in shared memory buffer.
    /// Then the registration shall be tried again by the caller later.
    /// Due to the lock-free behavior, there is a possibility that a type might be registered
    /// multiple times and thus have multiple allowed TypeIdentifiers. Datarouter shall tolerate this and accept any
    /// registered type identifier.
    ///
    /// This method is thread-safe, lock-free and wait-free.
    template <typename Typeinfo>
    score::cpp::optional<TypeIdentifier> TryRegisterType(const Typeinfo& info) noexcept
    {
        score::cpp::optional<TypeIdentifier> result{};

        constexpr size_type kTypeIdentifierSize = sizeof(TypeIdentifier);
        const auto type_info_size_pre = info.size();
        static_assert(std::is_same<decltype(type_info_size_pre), const std::size_t>::value,
                      "Return type of ::size() method of template parameter has uncompatible type");

        //  static_cast of a positive value after value has been verified
        const auto type_info_size = static_cast<size_type>(type_info_size_pre);
        //  cast to bigger type:
        const auto total_size = static_cast<Length>(kTypeIdentifierSize) + static_cast<Length>(type_info_size);

        this->AllocAndWrite(
            TimePoint::clock::now(),
            GetRegisterTypeToken(),
            total_size,
            [this, &info, &result, type_info_size](const score::cpp::span<Byte> payload_span) noexcept {
                // Write type identifier
                result = type_identifier_.fetch_add(1UL);
                // static cast is allowed as negative value of size type is not possible and maximum size is asserted
                // when doesn't meet requirements
                // Suppress "AUTOSAR C++14 M5-2-8" rule. The rule declares:
                // An object with integer type or pointer to void type shall not be converted to an object with pointer
                // type.
                // But we need to convert void pointer to bytes for serialization purposes, no out of bounds there
                // coverity[autosar_cpp14_m5_2_8_violation]
                const score::cpp::span<Byte> result_span{static_cast<Byte*>(static_cast<void*>(&result)), sizeof(result)};
                std::ignore = std::copy_n(
                    result_span.begin(), static_cast<std::size_t>(kTypeIdentifierSize), payload_span.begin());

                // Write type info
                const auto type_info_span = payload_span.subspan(kTypeIdentifierSize, type_info_size);
                info.Copy(type_info_span);
            });

        return result;
    }

    /// \brief Toggles the buffer active for writing and returns buffer that is intended for reading when released by
    /// writers.
    ///
    /// This method is thread safe only against AllocAndWrite() and TryRegisterType().
    /// This method shall not be called from multiple threads.
    ReadAcquireResult ReadAcquire() noexcept;

    /// \brief Signals to Datarouter to switch to detached mode.
    ///
    /// This method is thread-safe and wait-free.
    void DetachWriter() noexcept;

    /// \brief Increments the counter for type registration failures.
    ///
    /// This method is thread-safe and wait-free.
    void IncrementTypeRegistrationFailures() noexcept;

  private:
    SharedData& shared_data_;
    WaitFreeAlternatingWriter alternating_writer_;
    AlternatingReaderProxy alternating_reader_;
    UnmapCallback unmap_callback_;
    std::atomic<TypeIdentifier> type_identifier_;
    bool moved_from_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_SHARED_MEMORY_WRITER_H
