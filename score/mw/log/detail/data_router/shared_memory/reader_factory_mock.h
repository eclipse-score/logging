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

#ifndef BMW_MW_LOG_WAIT_FREE_SHARED_MEMORY_READER_FACTORY_MOCK
#define BMW_MW_LOG_WAIT_FREE_SHARED_MEMORY_READER_FACTORY_MOCK

#include "score/mw/log/detail/data_router/shared_memory/reader_factory.h"

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief The factory is responsible for creating the shared memory file and instantiating the SharedMemoryReader
class ReaderFactoryMock : public ReaderFactory
{
  public:
    MOCK_METHOD((std::optional<SharedMemoryReader>),
                Create,
                (const std::int32_t file_handle, const pid_t expected_pid),
                (override));
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  //  BMW_MW_LOG_WAIT_FREE_SHARED_MEMORY_READER_FACTORY_MOCK
