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

#ifndef BMW_MW_LOG_SHARED_MEMORY_READER_FACTORY_IMPL
#define BMW_MW_LOG_SHARED_MEMORY_READER_FACTORY_IMPL

#include "score/mw/log/detail/data_router/shared_memory/reader_factory.h"

#include "score/os/mman.h"
#include "score/os/stat.h"

#include "score/optional.hpp"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief The factory is responsible for opening the shared memory file and instantiating the SharedMemoryReader
class ReaderFactoryImpl : public ReaderFactory
{
  public:
    explicit ReaderFactoryImpl(score::cpp::pmr::unique_ptr<score::os::Mman>&& mman,
                               score::cpp::pmr::unique_ptr<score::os::Stat>&& stat_osal) noexcept;
    std::unique_ptr<ISharedMemoryReader> Create(const std::int32_t file_descriptor,
                                                const pid_t expected_pid) noexcept override;

  private:
    score::cpp::pmr::unique_ptr<score::os::Mman> mman_;
    score::cpp::pmr::unique_ptr<score::os::Stat> stat_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  //  BMW_MW_LOG_SHARED_MEMORY_READER_FACTORY_IMPL
