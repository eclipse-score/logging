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

#ifndef BMW_MW_LOG_SHARED_MEMORY_WRITER_FACTORY
#define BMW_MW_LOG_SHARED_MEMORY_WRITER_FACTORY

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_writer.h"

#include "score/os/fcntl.h"
#include "score/os/mman.h"
#include "score/os/stat.h"
#include "score/os/stdlib.h"
#include "score/os/unistd.h"

#include "score/optional.hpp"
#include <string_view>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

struct LoggingClientFileNameResult
{
    /*
    Maintaining compatibility and avoiding performance overhead outweighs POD Type (class) based design for this
    particular struct. The Type is simple and does not require invariance (interface OR custom behavior) as per the
    design. Moreover the type is ONLY used internally under the namespace detail and NOT exposed publicly; this is
    additionally guaranteed by the build system(bazel) visibility
  */
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::string file_name;
    // coverity[autosar_cpp14_m11_0_1_violation]
    std::string identifier;
};

/// \brief The factory is responsible for creating the shared memory file and instantiating the SharedMemoryWriter
class WriterFactory
{
  public:
    struct OsalInstances
    {
        /*
          Maintaining compatibility and avoiding performance overhead outweighs POD Type (class) based design for this
          particular struct. The Type is simple and does not require invariance (interface OR custom behavior) as per
          the design. Moreover the type is ONLY used internally under the namespace detail and NOT exposed publicly;
          this is additionally guaranteed by the build system(bazel) visibility
        */
        // coverity[autosar_cpp14_m11_0_1_violation]
        score::cpp::pmr::unique_ptr<score::os::Fcntl> fcntl_osal{};
        // coverity[autosar_cpp14_m11_0_1_violation]
        score::cpp::pmr::unique_ptr<score::os::Unistd> unistd{};
        // coverity[autosar_cpp14_m11_0_1_violation]
        score::cpp::pmr::unique_ptr<score::os::Mman> mman{};
        // coverity[autosar_cpp14_m11_0_1_violation]
        score::cpp::pmr::unique_ptr<score::os::Stat> stat_osal{};
        // coverity[autosar_cpp14_m11_0_1_violation]
        score::cpp::pmr::unique_ptr<score::os::Stdlib> stdlib{};
    };

    explicit WriterFactory(OsalInstances osal) noexcept;
    score::cpp::optional<SharedMemoryWriter> Create(const std::size_t ring_buffer_size,
                                             const bool dynamic_mode,
                                             const std::string_view app_id) noexcept;

    std::string GetIdentifier() const noexcept;
    std::string GetFileName() const noexcept;

  private:
    LoggingClientFileNameResult GetStaticLoggingClientFilename(const std::string_view app_id) const noexcept;
    void UnlinkExistingFile(const std::string& file_name) const noexcept;
    score::cpp::optional<int32_t> OpenAndTruncateFile(const std::size_t buffer_total_size,
                                               const std::string& file_name,
                                               const score::os::Fcntl::Open flags) noexcept;
    score::cpp::optional<void* const> MapSharedMemory(const std::size_t buffer_total_size,
                                               const int32_t memfd_write,
                                               const std::string& file_name) noexcept;
    bool IsMemoryAligned(void* const ring_buffer_address) noexcept;
    SharedData* ConstructSharedData(void* const ring_buffer_address, const std::size_t ring_buffer_size) const noexcept;
    LoggingClientFileNameResult PrepareFileNameAndUpdateOpenFlags(score::os::Fcntl::Open& file_open_flags,
                                                                  const bool dynamic_mode,
                                                                  const std::string_view app_id) const noexcept;
    score::cpp::optional<void* const> GetAlignedRingBufferAddress(const std::size_t total_size,
                                                           const std::string& file_name,
                                                           const score::os::Fcntl::Open file_open_flags) noexcept;

    OsalInstances osal_;
    score::cpp::expected<void*, score::os::Error> mmap_result_;
    UnmapCallback unmap_callback_;
    LoggingClientFileNameResult file_attributes_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif
