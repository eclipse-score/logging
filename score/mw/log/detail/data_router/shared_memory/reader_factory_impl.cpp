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

#include "score/mw/log/detail/data_router/shared_memory/reader_factory_impl.h"

#include <iostream>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

ReaderFactoryImpl::ReaderFactoryImpl(score::cpp::pmr::unique_ptr<score::os::Mman>&& mman,
                                     score::cpp::pmr::unique_ptr<score::os::Stat>&& stat_osal) noexcept
    : ReaderFactory(), mman_{std::move(mman)}, stat_{std::move(stat_osal)}
{
}

Byte* GetBufferAddress(Byte* const start, const Length offset)
{
    auto* start_address = start;
    std::advance(start_address, static_cast<std::iterator_traits<Byte*>::difference_type>(offset));
    return start_address;
}

std::unique_ptr<ISharedMemoryReader> ReaderFactoryImpl::Create(const std::int32_t file_descriptor,
                                                               const pid_t expected_pid) noexcept
{
    score::os::StatBuffer buffer{};

    const auto stat_result = stat_->fstat(file_descriptor, buffer);

    if (stat_result.has_value() == false)
    {
        std::cerr << "ReaderFactoryImpl::Create: fstat failed: " << stat_result.error();
        return nullptr;
    }

    if (buffer.st_size < 0)
    {
        std::cerr << "ReaderFactoryImpl::Create: unexpected negative buffer.st_size: " << buffer.st_size;
        return nullptr;
    }

    const auto map_size_bytes = static_cast<Length>(buffer.st_size);

    if (map_size_bytes < sizeof(SharedData))
    {
        std::cerr << "ReaderFactoryImpl::Create: Invalid shared memory size: found " << map_size_bytes
                  << " but expected at least " << sizeof(SharedData) << " bytes\n";
        return nullptr;
    }

    static constexpr void* kNullAddr = nullptr;
    static constexpr std::int64_t kMmapOffset = 0;
    auto mmap_result = mman_->mmap(kNullAddr,
                                   map_size_bytes,
                                   score::os::Mman::Protection::kRead,
                                   score::os::Mman::Map::kShared,
                                   file_descriptor,
                                   kMmapOffset);

    if (mmap_result.has_value() == false)
    {
        std::cerr << "ReaderFactoryImpl::Create: mmap failed: " << mmap_result.error() << '\n';
        return nullptr;
    }

    /*
        Deviation from Rule M5-2-8:
        - An object with integer type or pointer to void type shall not be converted
          to an object with pointer type.
        Justification:
        - casted as SharedData to access memory mapped by mmap function.
    */
    // coverity[autosar_cpp14_m5_2_8_violation]
    const SharedData& shared_data = *(static_cast<const SharedData*>(mmap_result.value()));

    const auto max_offset_bytes = std::max(
        shared_data.linear_buffer_1_offset + GetDataSizeAsLength(shared_data.control_block.control_block_even.data),
        shared_data.linear_buffer_2_offset + GetDataSizeAsLength(shared_data.control_block.control_block_odd.data));

    UnmapCallback unmap_callback = [mman = std::move(mman_), address = mmap_result.value(), map_size_bytes]() {
        const auto munmap_result = mman->munmap(address, map_size_bytes);
        if (munmap_result.has_value() == false)
        {
            std::cerr << "UnmapCallback: failed to unmap: " << munmap_result.error()
                      << '\n';  // LCOV_EXCL_BR_LINE: there are no branches to be covered here.
        }
    };

    if (max_offset_bytes > map_size_bytes)
    {
        std::cerr << "ReaderFactoryImpl::Create: Invalid shared_data content: max_offset_bytes=" << max_offset_bytes
                  << " but map_size_bytes is only " << map_size_bytes << '\n';
        unmap_callback();
        return nullptr;
    }

    if (shared_data.producer_pid != expected_pid)
    {
        std::cerr << "SharedMemoryReader found invalid pid. Expected " << expected_pid << " but found "
                  << shared_data.producer_pid << " in shared memory. Dropping the logs from this client.\n";
        unmap_callback();
        return nullptr;
    }
    /*
        Deviation from Rule M5-2-8:
        - An object with integer type or pointer to void type shall not be converted
          to an object with pointer type.
        Justification:
        - casted as Byte to access memory mapped by mmap function.
    */
    // coverity[autosar_cpp14_m5_2_8_violation]
    auto* const shared_data_addr = static_cast<Byte*>(mmap_result.value());
    auto* const buffer1_addr = GetBufferAddress(shared_data_addr, shared_data.linear_buffer_1_offset);
    auto* const buffer2_addr = GetBufferAddress(shared_data_addr, shared_data.linear_buffer_2_offset);
    const score::cpp::span<Byte> buffer_block_even(buffer1_addr, shared_data.control_block.control_block_even.data.size());
    const score::cpp::span<Byte> buffer_block_odd(buffer2_addr, shared_data.control_block.control_block_odd.data.size());

    AlternatingReadOnlyReader alternating_read_only_reader{
        shared_data.control_block, buffer_block_even, buffer_block_odd};

    return std::make_unique<SharedMemoryReader>(
        shared_data, std::move(alternating_read_only_reader), std::move(unmap_callback));
}

ReaderFactoryPtr ReaderFactory::Default(score::cpp::pmr::memory_resource* memory_resource) noexcept
{
    if (memory_resource == nullptr)
    {
        std::cerr << "ERROR! ReaderFactory::Default(): Memory resource is null pointer\n";
        return nullptr;
    }

    return std::make_unique<ReaderFactoryImpl>(score::os::Mman::Default(memory_resource),
                                               score::os::Stat::Default(memory_resource));
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
