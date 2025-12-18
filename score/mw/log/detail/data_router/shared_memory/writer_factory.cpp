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

#include "score/mw/log/detail/data_router/shared_memory/writer_factory.h"

#include <iostream>
#include <memory>
#include <sstream>

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
constexpr auto kSeperator = ".";
// NOLINTNEXTLINE(modernize-avoid-c-arrays) size available in compile time and bounds are checked
constexpr char kFileNameTemplate[] = "/tmp/logging-XXXXXX.shmem";
// NOLINTNEXTLINE(modernize-avoid-c-arrays) size available in compile time and bounds are checked
constexpr char kFileNameDirectoryTemplate[] = "/tmp/";
// NOLINTNEXTLINE(modernize-avoid-c-arrays) size available in compile time and bounds are checked
constexpr char kFileNameBaseTemplate[] = "logging-XXXXXX";
// NOLINTNEXTLINE(modernize-avoid-c-arrays) size available in compile time and bounds are checked
constexpr char kSuffixName[] = ".shmem";
static_assert(sizeof(kSuffixName) <= static_cast<std::size_t>(std::numeric_limits<int32_t>::max() - 1),
              "size of kSuffixName is too big");
constexpr int32_t kSizeOfTemplateSuffix{static_cast<int32_t>(sizeof(kSuffixName)) - 1};

}  // namespace

LoggingClientFileNameResult WriterFactory::GetStaticLoggingClientFilename(const std::string_view app_id) const noexcept
{
    const auto uid = osal_.unistd->getuid();
    std::stringstream logging_id;
    logging_id << "logging" << kSeperator << std::string{app_id.data(), app_id.size()} << kSeperator << uid;
    std::stringstream file_name;
    constexpr auto kFileNameDirectoryTemplateSize = sizeof(kFileNameDirectoryTemplate) - 1UL;
    file_name << std::string{std::begin(kFileNameDirectoryTemplate), kFileNameDirectoryTemplateSize} << logging_id.str()
              << std::begin(kSuffixName);
    LoggingClientFileNameResult result{};
    result.file_name = file_name.str();
    result.identifier = logging_id.str();
    return result;
}

WriterFactory::WriterFactory(OsalInstances osal) noexcept : osal_(std::move(osal)) {}

void WriterFactory::UnlinkExistingFile(const std::string& file_name) const noexcept
{
    //  Check and unlink the file to possibly avoid destroying the content by opening it again.
    //  This should allow any other processes finish their work uninterrupted.
    if (osal_.unistd->access(file_name.c_str(), score::os::Unistd::AccessMode::kExists).has_value())
    {
        std::cerr << "Logging shared memory file: '" << file_name << "' already exists" << '\n';
        const auto unlink_result = osal_.unistd->unlink(file_name.c_str());
        if (false == unlink_result.has_value())
        {
            std::cerr << "Unlinking of '" << file_name << "' failed with code: " << unlink_result.error() << '\n';
        }
    }
}

score::cpp::optional<int32_t> WriterFactory::OpenAndTruncateFile(const std::size_t buffer_total_size,
                                                          const std::string& file_name,
                                                          const score::os::Fcntl::Open flags) noexcept
{
    const score::cpp::span<const char> funcSpan(__func__);
    constexpr auto kOpenModeFlags =
        score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kReadGroup | score::os::Stat::Mode::kReadOthers;
    // NOLINTNEXTLINE(score-banned-function) it is among safety headers.
    const auto open_ret_val = osal_.fcntl_osal->open(file_name.c_str(), flags, kOpenModeFlags);
    if (open_ret_val.has_value() == false)
    {
        std::cerr << funcSpan.data() << ":open " << file_name << " " << open_ret_val.error().ToString() << '\n';
        return {};
    }
    const int32_t memfd_write_ = open_ret_val.value();

    const auto chmod_ret_val = osal_.stat_osal->chmod(file_name.c_str(), kOpenModeFlags);
    if (chmod_ret_val.has_value() == false)
    {
        std::cerr << funcSpan.data() << ":chmod " << file_name << " " << chmod_ret_val.error().ToString() << '\n';
        return {};
    }

    auto ftruncate_ret_val = osal_.unistd->ftruncate(memfd_write_, static_cast<off_t>(buffer_total_size));
    if (ftruncate_ret_val.has_value() == false)
    {
        std::cerr << funcSpan.data() << ":ftruncate " << ftruncate_ret_val.error().ToString() << '\n';
        std::ignore = osal_.unistd->unlink(file_name.c_str());
        return {};
    }
    return memfd_write_;
}

score::cpp::optional<void* const> WriterFactory::MapSharedMemory(const std::size_t buffer_total_size,
                                                          const int32_t memfd_write,
                                                          const std::string& file_name) noexcept
{
    mmap_result_ = osal_.mman->mmap(nullptr,
                                    static_cast<size_t>(buffer_total_size),
                                    score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                                    score::os::Mman::Map::kShared,
                                    memfd_write,
                                    0);

    if (!mmap_result_.has_value())
    {
        std::cerr << "MwsrWriterImpl:mmap " << mmap_result_.error().ToString() << '\n';
        static_cast<void>(osal_.unistd->unlink(file_name.c_str()));
        return {};
    }

    unmap_callback_ = [mman = std::move(osal_.mman), address = mmap_result_.value(), size = buffer_total_size]() {
        auto munmap_result = mman->munmap(address, size);
        if (munmap_result.has_value() == false)
        {
            std::cerr << "UnmapCallback: failed to unmap: " << munmap_result.error()
                      << '\n';  // LCOV_EXCL_BR_LINE: there are no branches to be covered here.
        }
    };

    if (nullptr == mmap_result_.value())
    {
        const score::cpp::span<const char> funcSpan(__func__);
        std::cerr << funcSpan.data() << ":mmap result it nullptr\n";
        unmap_callback_();
        return {};
    }
    return mmap_result_.value();
}

bool WriterFactory::IsMemoryAligned(void* const ring_buffer_address) noexcept
{
    bool result = true;
    constexpr auto kAlignRequirement = std::alignment_of<SharedData>::value;

    static_assert(sizeof(Byte) == 1UL, "Pointer arithmetic is not valid");
    static_assert(sizeof(Byte) < sizeof(Length), "Alignment assumption are not valid");

    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast) justified above
    // Cast is used just for sanity check
    // coverity[autosar_cpp14_a5_2_4_violation]
    // coverity[autosar_cpp14_m5_2_9_violation]
    if (0UL != (reinterpret_cast<uintptr_t>(ring_buffer_address) % kAlignRequirement))
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast) justified above
    {
        std::cerr << "Shared memory missaligned" << '\n';
        unmap_callback_();
        result = false;
    }
    return result;
}

// checking ring_buffer_address in caller function (WriterFactory::Create)
// and it uses  ring_buffer_address as score::cpp::optional so we check it first before passing it to function.
// coverity[autosar_cpp14_a8_4_10_violation]
SharedData* WriterFactory::ConstructSharedData(void* const ring_buffer_address,
                                               const std::size_t ring_buffer_size) const noexcept
{
    // NOLINTNEXTLINE(score-no-dynamic-raw-memory) new keyword is intended by design
    auto shared_data = new (ring_buffer_address) SharedData();
    shared_data->producer_pid = osal_.unistd->getpid();

    //  move pointer to point after shared data structure and only after that cast to void* for further pointer
    //  operations:
    //  Cast used because by design we cast data structures onto memory allocated by mmap
    auto iter = shared_data;
    std::advance(iter, 1); /*moving pointer forward by one SharedData  type*/
    void* const linear_space = static_cast<void*>(iter);

    using SpanSizeType = score::cpp::span<Byte>::size_type;
    using LocalSizeType = std::remove_cv<decltype(ring_buffer_size)>::type;
    const LocalSizeType half_buffer_size = ring_buffer_size / 2UL;

    //  Cast to bigger type just for checking safty of other casts
    static_assert((std::numeric_limits<LocalSizeType>::max() / 2UL) <=
                      static_cast<std::uint64_t>(std::numeric_limits<SpanSizeType>::max()),
                  "Wrong type size");
    // Cast allowed as size values can not be negative and maximum value checked and asserted if it doesn't fit
    const auto linear_buffer_size = static_cast<SpanSizeType>(half_buffer_size);

    //  First linear buffer:
    // Suppress "AUTOSAR C++14 M5-2-8" rule. The rule declares:
    // An object with integer type or pointer to void type shall not be converted to an object with pointer type.
    // But we need to convert void pointer to bytes for serialization purposes, no out of bounds there
    // coverity[autosar_cpp14_m5_2_8_violation]
    const auto block_1_data = static_cast<Byte*>(linear_space);
    shared_data->control_block.control_block_even.data = score::cpp::span<Byte>{block_1_data, linear_buffer_size};
    std::ignore = InitializeSharedData(*shared_data);
    //  Initialize buffer switch sides:
    shared_data->linear_buffer_1_offset = sizeof(SharedData);

    //  Second linear buffer:
    // Suppress "AUTOSAR C++14 M5-2-8" rule. The rule declares:
    // An object with integer type or pointer to void type shall not be converted to an object with pointer type.
    // But we need to convert void pointer to bytes for serialization purposes, no out of bounds there
    // coverity[autosar_cpp14_m5_2_8_violation]
    auto block_2_data = static_cast<Byte*>(linear_space);

    std::advance(block_2_data, static_cast<std::ptrdiff_t>(half_buffer_size));

    shared_data->control_block.control_block_odd.data = score::cpp::span<Byte>{block_2_data, linear_buffer_size};
    shared_data->linear_buffer_2_offset = sizeof(SharedData) + half_buffer_size;
    return shared_data;
}

LoggingClientFileNameResult WriterFactory::PrepareFileNameAndUpdateOpenFlags(
    score::os::Fcntl::Open& file_open_flags,
    const bool dynamic_mode,
    const std::string_view app_id) const noexcept
{
    LoggingClientFileNameResult result_file_name;

    if (true == dynamic_mode)  //  Create dynamic identifier file
    {
        constexpr auto kFileNameTemplateSize = sizeof(kFileNameTemplate);
        std::array<char, kFileNameTemplateSize> name_buffer{};
        std::ignore = std::copy_n(std::begin(kFileNameTemplate), kFileNameTemplateSize, name_buffer.begin());
        const auto mkstemp_result = osal_.stdlib->mkstemps(name_buffer.data(), kSizeOfTemplateSuffix);
        if (!mkstemp_result.has_value())
        {
            std::cerr << "mkstemps: Failed to create '" << name_buffer.data() << "' file for app: " << app_id.data()
                      << '\n';
        }
        constexpr score::os::Stat::Mode permissions = score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kReadGroup |
                                                    score::os::Stat::Mode::kReadOthers | score::os::Stat::Mode::kWriteUser;

        if ((osal_.stat_osal->chmod(name_buffer.data(), permissions)).has_value() == false)
        {
            std::cerr << "Unable to apply permissions to: " << name_buffer.data() << '\n';
        }
        result_file_name.file_name = std::string{name_buffer.data(), name_buffer.size() - 1UL};

        auto find_beginning_logging_identifier = std::begin(name_buffer);
        static_assert(
            sizeof(kFileNameDirectoryTemplate) <= static_cast<std::size_t>(std::numeric_limits<int32_t>::max() - 1),
            "size of kFileNameDirectoryTemplate is greater than size of int");
        std::advance(find_beginning_logging_identifier, static_cast<int32_t>(sizeof(kFileNameDirectoryTemplate)) - 1);
        result_file_name.identifier =
            std::string{find_beginning_logging_identifier, sizeof(kFileNameBaseTemplate) - 1UL};
    }
    else
    {
        result_file_name = GetStaticLoggingClientFilename(app_id);
        //  only in deterministic mode
        UnlinkExistingFile(result_file_name.file_name);
        file_open_flags |= score::os::Fcntl::Open::kCreate;
    }

    return result_file_name;
}

score::cpp::optional<void* const> WriterFactory::GetAlignedRingBufferAddress(
    const std::size_t total_size,
    const std::string& file_name,
    const score::os::Fcntl::Open file_open_flags) noexcept
{
    const auto memfd_write_ = OpenAndTruncateFile(total_size, file_name, file_open_flags);
    if (memfd_write_.has_value() == false)
    {
        return {};
    }

    auto ring_buffer_address = MapSharedMemory(total_size, memfd_write_.value(), file_name);
    if (ring_buffer_address.has_value() == false)
    {
        return {};
    }

    if (IsMemoryAligned(ring_buffer_address.value()) == false)
    {
        return {};
    }

    return ring_buffer_address.value();
}

score::cpp::optional<SharedMemoryWriter> WriterFactory::Create(const std::size_t ring_buffer_size,
                                                        const bool dynamic_mode,
                                                        const std::string_view app_id) noexcept
{
    if ((((osal_.fcntl_osal == nullptr) || (osal_.unistd == nullptr)) || (osal_.mman == nullptr)) ||
        ((osal_.stat_osal == nullptr) || (osal_.stdlib == nullptr)))
    {
        return {};
    }

    auto flags =
        score::os::Fcntl::Open::kReadWrite | score::os::Fcntl::Open::kExclusive | score::os::Fcntl::Open::kCloseOnExec;
    file_attributes_ = PrepareFileNameAndUpdateOpenFlags(flags, dynamic_mode, app_id);

    constexpr std::size_t buffer_start_offset = sizeof(SharedData);
    if (buffer_start_offset > std::numeric_limits<size_t>::max() - ring_buffer_size)
    {
        const score::cpp::span<const char> funcSpan(__func__);
        std::cerr << "(buffer_start_offset + ring_buffer_size) Overflow happened in function : " << funcSpan.data()
                  << " in line : " << __LINE__ << "\n";
    }
    const std::size_t buffer_end_offset = buffer_start_offset + ring_buffer_size;
    const auto total_size = buffer_end_offset;

    auto ring_buffer_address = GetAlignedRingBufferAddress(total_size, file_attributes_.file_name, flags);
    if (ring_buffer_address.has_value() == false)
    {
        return {};
    }

    SharedMemoryWriter shared_memory_writer{*ConstructSharedData(ring_buffer_address.value(), ring_buffer_size),
                                            std::move(unmap_callback_)};
    return shared_memory_writer;
}

std::string WriterFactory::GetIdentifier() const noexcept
{
    return file_attributes_.identifier;
}
std::string WriterFactory::GetFileName() const noexcept
{
    return file_attributes_.file_name;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
