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

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_writer.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

SharedMemoryWriter::SharedMemoryWriter(SharedData& shared_data, UnmapCallback unmap_callback) noexcept
    : shared_data_{shared_data},
      alternating_writer_{shared_data.control_block},
      alternating_reader_{shared_data.control_block},
      unmap_callback_{std::move(unmap_callback)},
      type_identifier_{},
      moved_from_{}
{
}

// Suppress "AUTOSAR C++14 A12-8-4", The rule states: "Move constructor shall not initialize its class
// members and base classes using copy semantics".
// Suppressed: The underlying types are scalar, and shared_data_ is a reference.
SharedMemoryWriter::SharedMemoryWriter(SharedMemoryWriter&& other) noexcept
    : shared_data_{other.shared_data_},
      // coverity[autosar_cpp14_a12_8_4_violation]
      alternating_writer_{shared_data_.control_block},
      // coverity[autosar_cpp14_a12_8_4_violation]
      alternating_reader_{shared_data_.control_block},
      unmap_callback_{std::move(other.unmap_callback_)},
      // coverity[autosar_cpp14_a12_8_4_violation]
      // coverity[autosar_cpp14_a18_9_2_violation : FALSE]
      type_identifier_{other.type_identifier_.load()},
      moved_from_{other.moved_from_}  // LCOV_EXCL_BR_LINE
{
    other.moved_from_ = true;
}

ReadAcquireResult SharedMemoryWriter::ReadAcquire() noexcept
{
    const auto acquired = alternating_reader_.Switch();
    ReadAcquireResult result{};
    result.acquired_buffer = acquired;
    return result;
}

void SharedMemoryWriter::DetachWriter() noexcept
{
    shared_data_.writer_detached.store(true);
}

void SharedMemoryWriter::IncrementTypeRegistrationFailures() noexcept
{
    std::ignore = shared_data_.number_of_drops_type_registration_failed.fetch_add(1U);
}

SharedMemoryWriter::~SharedMemoryWriter() noexcept
{
    if (moved_from_)
    {
        return;
    }
    DetachWriter();
    if (!unmap_callback_.empty())
    {
        unmap_callback_();
    }
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
