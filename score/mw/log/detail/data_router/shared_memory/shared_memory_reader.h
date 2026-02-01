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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_SHARED_MEMORY_READER_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_SHARED_MEMORY_READER_H

#include "score/mw/log/detail/data_router/shared_memory/i_shared_memory_reader.h"
#include "score/mw/log/detail/wait_free_producer_queue/alternating_reader.h"

#include <score/utility.hpp>

#include <cstring>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief This class manages the reading of serialized data types on read-only shared memory.
/// This class is not thread safe.
class SharedMemoryReader : public ISharedMemoryReader
{
  public:
    explicit SharedMemoryReader(const SharedData& shared_data,
                                AlternatingReadOnlyReader alternating_read_only_reader,
                                UnmapCallback unmap_callback) noexcept;

    ~SharedMemoryReader();

    SharedMemoryReader(const SharedMemoryReader&) = delete;
    SharedMemoryReader(SharedMemoryReader&&) noexcept;
    SharedMemoryReader& operator=(const SharedMemoryReader&) = delete;
    SharedMemoryReader& operator=(SharedMemoryReader&&) = delete;

    /// \brief Returns the data acquired through a prior call to NotifyAcquisitionSetReader()
    /// This method is handling detached mode, which accesses the part of the shared memory currently assigned to
    /// writers based on the assumption that the writer has already finished any activities leading to data
    /// modification. In this case it is assumed that logging client has terminated or crashed.
    std::optional<Length> Read(const TypeRegistrationCallback& type_registration_callback,
                               const NewRecordCallback& new_message_callback) noexcept override;

    //  This function may be used to get a temporary view of the value of bytes acquired by writers.
    std::optional<Length> PeekNumberOfBytesAcquiredInBuffer(
        const std::uint32_t acquired_buffer_count_id) const noexcept override;

    /// \brief Method shall be called when a client closed the connection to Datarouter.
    std::optional<Length> ReadDetached(const TypeRegistrationCallback& type_registration_callback,
                                       const NewRecordCallback& new_message_callback) noexcept override;

    Length GetNumberOfDropsWithBufferFull() const noexcept override;
    Length GetNumberOfDropsWithInvalidSize() const noexcept override;
    Length GetNumberOfDropsWithTypeRegistrationFailed() const noexcept override;
    Length GetSizeOfDropsWithBufferFull() const noexcept override;

    Length GetRingBufferSizeBytes() const noexcept override;

    bool IsBlockReleasedByWriters(const std::uint32_t block_count) noexcept override;

    /// \brief This method shall be called by the server when a client has acknowledged an acquire request.
    /// It sets Reader to acquired data that can be later used by Read() method
    /// Returns number of bytes of acquired buffer if available. Otherwise it returns std::nullopt
    std::optional<Length> NotifyAcquisitionSetReader(const ReadAcquireResult& acquire_result) noexcept override;

  private:
    const SharedData& shared_data_;
    UnmapCallback unmap_callback_;

    std::optional<LinearReader> linear_reader_;
    std::optional<ReadAcquireResult> acquired_data_;
    Length number_of_acquired_bytes_;
    bool finished_reading_after_detach_;
    std::uint32_t buffer_expected_to_read_next_;
    bool is_writer_detached_;
    AlternatingReadOnlyReader alternating_read_only_reader_;

    /// \brief Method shall be called when a client closed the connection to Datarouter.
    /// The next call to Read() will return the data from both buffers.
    void DetachWriter() noexcept;
    bool IsWriterDetached() const noexcept;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_SHARED_MEMORY_SHARED_MEMORY_READER_H
