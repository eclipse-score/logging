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

#ifndef BMW_MW_LOG_SHARED_MEMORY_READER_FACTORY
#define BMW_MW_LOG_SHARED_MEMORY_READER_FACTORY

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"

#include "score/memory.hpp"

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class ReaderFactory;
using ReaderFactoryPtr = std::unique_ptr<ReaderFactory>;

/// \brief The factory is responsible for creating the shared memory file and instantiating the SharedMemoryReader
class ReaderFactory
{
  public:
    virtual std::unique_ptr<ISharedMemoryReader> Create(const std::int32_t file_descriptor,
                                                        const pid_t expected_pid) = 0;

    ReaderFactory() = default;
    virtual ~ReaderFactory() = default;
    ReaderFactory(const ReaderFactory&) = delete;
    ReaderFactory(ReaderFactory&&) = delete;
    ReaderFactory& operator=(ReaderFactory&&) = delete;
    ReaderFactory& operator=(const ReaderFactory&) = delete;

    static ReaderFactoryPtr Default(score::cpp::pmr::memory_resource* memory_resource) noexcept;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  //  BMW_MW_LOG_SHARED_MEMORY_READER_FACTORY
