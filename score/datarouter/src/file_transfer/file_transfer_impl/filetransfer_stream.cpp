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

#include "filetransfer_stream.h"
#include "dlt/dlt_headers.h"
#include "score/os/stat.h"
#include "score/os/stdio.h"
#include "score/os/utils/thread.h"
#include "score/mw/log/logging.h"

#include <sys/stat.h>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace score
{
namespace logging
{
namespace dltserver
{

FileTransferStreamHandler::FileTransferStreamHandler(IOutput& output)
    : LogParser::TypeHandler(), exit_requested_{false}, serialno_(0), fsize_(0), packagecount_(0), output_(output)
{
    filetransfer_thread_ = score::cpp::jthread([this] {
        this->ProcessFileTransfer();
    });
    score::os::set_thread_name(filetransfer_thread_, "file_transfer");
}

/*
Deviation from Rule A15-5-1:
- All user-provided class destructors, deallocation functions, move constructors,
- move assignment operators and swap functions shall not exit with an exception.
- A noexcept exception specification shall be added to these functions as appropriate.
Justification:
- Ensure that filetransfer_thread is not running after destruction of FileTransferStreamHandler
- checking filetransfer_thread.joinable() should be enough to avoid exception from join().
- in this case join() could throw exception only if something goes wrong on OS level.
- this should be fine, moreover it could happen only on system shutdown stage
- and does not affect normal runtime
*/
// coverity[autosar_cpp14_a15_5_1_violation] see above
FileTransferStreamHandler::~FileTransferStreamHandler() noexcept
{
    exit_requested_.store(true);

    if (filetransfer_thread_.joinable())
    {
        filetransfer_thread_.join();
    }
}
// Suppress "AUTOSAR C++14 A3-1-1", the rule states: " It shall be possible to include any header file in multiple
// translation units without violating the One Definition Rule."
// False positive. Its defined in .cpp file and only declared in .h file
// coverity[autosar_cpp14_a3_1_1_violation : FALSE]
void FileTransferStreamHandler::Handle(TimestampT /* timestamp */, const char* data, BufsizeT size)
{
    if (!output_.IsOutputEnabled())
    {
        return;
    }
    score::logging::FileTransferEntry entry{};
    using S = ::score::common::visitor::logging_serializer;
    S::deserialize(data, size, entry);
    std::unique_lock<std::shared_timed_mutex> lock(filetransfer_mutex_);
    container_.push(entry);
    score::mw::log::LogInfo() << "Requested file transfer: " << entry.file_name << ", delete: " << entry.delete_file;
}

void FileTransferStreamHandler::ProcessFileTransfer()
{
    auto read_from_queue = [this]() {
        std::shared_lock<std::shared_timed_mutex> lock(filetransfer_mutex_);
        appid_ = container_.front().appid;
        ctxid_ = container_.front().ctxid;
        return container_.front().file_name;
    };
    auto container_not_empty = [this]() {
        std::shared_lock<std::shared_timed_mutex> lock(filetransfer_mutex_);
        return (!container_.empty());
    };
    while (!exit_requested_.load())
    {
        if (container_not_empty())
        {
            readfile_ = read_from_queue();
            if (LogFileHeader())
            {
                LogFileData();
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool FileTransferStreamHandler::ReadFileHeaderInfo(const std::string& filename)
{
    struct score::os::StatBuffer statbuf{};
    if (::score::os::Stat::instance().stat(filename.c_str(), statbuf).has_value())
    {
        auto get_file_serial_number = [](struct score::os::StatBuffer st, const std::string& file) {
            uint32_t hash = 0;
            if (file.empty())
            {
                // This check ensures robustness in GetFileSerialNumber,
                // but it cannot be triggered in practice because it's only called
                // after a successful stat() on a valid file. If stat() fails (e.g., empty path),
                // the function exits early before reaching this point.
                // LCOV_EXCL_START
                return hash;
                // LCOV_EXCL_STOP
            }
            hash = static_cast<uint32_t>(st.st_ino);
            hash = hash << (sizeof(hash) * 8U) / 2U;
            hash |= static_cast<uint32_t>(st.st_size);
            hash ^= static_cast<uint32_t>(st.ctime);
            for (std::string::const_iterator it = file.cbegin(); it != file.cend(); ++it)
            {
                hash = 53 * hash + static_cast<uint32_t>(*it);
            }
            return hash;
        };

        auto get_file_size = [](struct score::os::StatBuffer st) {
            return static_cast<uint32_t>(st.st_size);
        };

        auto get_file_creation_date = [](struct score::os::StatBuffer st) {
            struct tm tm_buf{};
            struct tm* ts = localtime_r(&st.ctime, &tm_buf);
            if (ts != nullptr)
            {
                std::ostringstream oss;
                oss << std::put_time(ts, "%c");
                return oss.str();
            }
            // This fallback path is only executed if `localtime_r` returns nullptr,
            // which can only happen on internal runtime failure (e.g., invalid time data).
            // However, since `localtime_r` is a C library function, it is not virtual and cannot be overridden
            // directly. The project uses `std::localtime_r` (from <ctime>) instead of the Time wrapper interface
            // (score::os::Time). As a result, Google Mock cannot intercept the call, and this branch cannot be covered in
            // tests. To cover it, a redesign would be needed to wrap all system time calls through mockable interfaces.
            // LCOV_EXCL_START
            return std::string("");
            // LCOV_EXCL_STOP
        };

        auto getpackages_count = [](const std::string&, uint32_t fsize) {
            uint32_t packages = 1U;
            if (fsize < kBufferSize)
            {
                return packages;
            }
            else
            {
                packages = fsize / kBufferSize;
                if (fsize % kBufferSize == 0)
                {
                    return packages;
                }
                else
                {
                    return packages + 1U;
                }
            }
        };

        serialno_ = get_file_serial_number(statbuf, filename);
        fsize_ = get_file_size(statbuf);
        creationdate_ = get_file_creation_date(statbuf);
        packagecount_ = getpackages_count(filename, fsize_);
        return true;
    }
    return false;
}

score::cpp::span<const std::uint8_t> CastDataSpanToConst(score::cpp::span<char> data)
{
    return score::cpp::span<const std::uint8_t>{
        /*
            Deviation from Rule M5-2-8:
            - Rule M5-2-8 (required, implementation, automated)
            An object with integer type or pointer to void type shall not be converted
            to an object with pointer type.
            Justification:
            - This is safe since we convert void data object to it's raw.
        */
        // coverity[autosar_cpp14_m5_2_8_violation]
        const_cast<const std::uint8_t*>(static_cast<std::uint8_t*>(static_cast<void*>(data.data()))),
        data.size()};
}

bool FileTransferStreamHandler::LogFileHeader()
{
    if (ReadFileHeaderInfo(readfile_) && !exit_requested_.load())
    {
        std::array<char, kBufferSize> buffer{};
        auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
        const auto result = PackageFileHeader(data_span, serialno_, readfile_, fsize_, creationdate_, packagecount_);
        if (result.has_value())
        {
            TimestampT time_stamp = TimestampT::clock::now();
            uint32_t tmsp = std::chrono::duration_cast<DltDurationT>(time_stamp.time_since_epoch()).count();
            const auto& [data_output, number_of_args] = result.value();
            output_.SendFtVerbose(
                CastDataSpanToConst(data_output), mw::log::LogLevel::kInfo, appid_, ctxid_, number_of_args, tmsp);
            return true;
        }
    }
    LogFileError(kDltFiletransferErrorFileHead);
    return false;
}

void FileTransferStreamHandler::LogFileData()
{
    FILE* file = nullptr;
    auto ret_fopen = score::os::Stdio::instance().fopen(readfile_.c_str(), "rb");
    if (ret_fopen.has_value())
    {
        file = ret_fopen.value();
    }
    if (file != nullptr && !exit_requested_.load())
    {
        std::array<char, kDatapkgsize> buffer{};
        for (uint32_t pkgno = 1U; pkgno <= packagecount_; pkgno++)
        {
            buffer.fill(0);
            auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
            const auto result = PackageFileData(data_span, file, serialno_, pkgno);
            if (not result.has_value())
            {
                // This line is not covered because `PackageFileData` returns std::nullopt
                // only when the provided buffer is too small. Since the production code
                // uses a fixed-size buffer (`kDatapkgsize`) that is always large enough,
                // this condition cannot be triggered as kDatapkgsize is hardcoded.
                // LCOV_EXCL_START
                score::os::Stdio::instance().fclose(file);
                LogFileError(kDltFiletransferErrorFileData, "Unable to format data package");
                return;
                // LCOV_EXCL_STOP
            }
            else
            {
                TimestampT time_stamp = TimestampT::clock::now();
                const auto& [data_packet, number_of_args] = result.value();
                uint32_t tmsp = std::chrono::duration_cast<DltDurationT>(time_stamp.time_since_epoch()).count();
                output_.SendFtVerbose(
                    CastDataSpanToConst(data_packet), mw::log::LogLevel::kInfo, appid_, ctxid_, number_of_args, tmsp);
            }
        }
        score::os::Stdio::instance().fclose(file);
        LogFileEnd();
    }
    else
    {
        // This fallback is only triggered if `fopen` fails or `PackageFileData` returns std::nullopt.
        // In production, a fixed-size buffer (`kDatapkgsize`) is always large enough for packaging,
        // so `PackageFileData` will not fail under normal test conditions.
        // Covering this would require artificial failure injection or a redesign for test hooks.
        // LCOV_EXCL_START
        LogFileError(kDltFiletransferErrorFileData);
        // LCOV_EXCL_STOP
    }
}

void FileTransferStreamHandler::LogFileEnd()
{
    if (!exit_requested_.load())
    {
        std::array<char, kBufferSize> buffer{};
        auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
        auto remove_from_file_queue = [this]() {
            std::unique_lock<std::shared_timed_mutex> lock(filetransfer_mutex_);
            if (!container_.empty())
            {
                if (0U != container_.front().delete_file)
                {
                    std::remove(readfile_.c_str());
                }
                container_.pop();
            }
            else
            {
                // This error log is only triggered if LogFileEnd() is called when the container is already empty.
                // Under normal usage (after a successful handle + transfer), the container is not empty- is not good.
                // because it has dynamic content and could be empty (misuse or race conditions ).
                // LCOV_EXCL_START
                std::perror(" Cannot remove file from empty Container !!! ");
                // LCOV_EXCL_STOP
            }
        };
        const auto result = PackageFileEnd(data_span, serialno_);
        if (result.has_value())
        {
            const auto [data_output, number_of_args] = result.value();
            TimestampT time_stamp = TimestampT::clock::now();
            uint32_t tmsp = std::chrono::duration_cast<DltDurationT>(time_stamp.time_since_epoch()).count();
            output_.SendFtVerbose(
                CastDataSpanToConst(data_output), mw::log::LogLevel::kInfo, appid_, ctxid_, number_of_args, tmsp);
        }
        remove_from_file_queue();
    }
}

void FileTransferStreamHandler::LogFileError(int16_t errorcode, std::string error_msg)
{
    if (!exit_requested_.load())
    {
        std::array<char, kBufferSize> buffer{};
        buffer.fill(0);
        auto remove_from_file_queue = [this]() {
            std::unique_lock<std::shared_timed_mutex> lock(filetransfer_mutex_);
            if (!container_.empty())
            {
                if (0U != container_.front().delete_file)
                {
                    std::remove(readfile_.c_str());
                }
                container_.pop();
            }
            else
            {
                // This error log is only triggered if LogFileEnd() is called when the container is already empty.
                // Under normal usage (after a successful handle + transfer), the container is not empty- is not good.
                // because it has dynamic content and could be empty (misuse or race conditions ).
                // condition, which should not happen by design.
                // LCOV_EXCL_START
                std::perror(" Cannot remove file from empty Container !!! ");
                // LCOV_EXCL_STOP
            }
        };
        auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
        const auto result = PackageFileError(
            data_span, errorcode, serialno_, readfile_, fsize_, creationdate_, packagecount_, error_msg);
        if (result.has_value())
        {
            TimestampT time_stamp = TimestampT::clock::now();
            uint32_t tmsp = std::chrono::duration_cast<DltDurationT>(time_stamp.time_since_epoch()).count();
            const auto& [data_output, number_of_args] = result.value();
            output_.SendFtVerbose(
                CastDataSpanToConst(data_output), mw::log::LogLevel::kError, appid_, ctxid_, number_of_args, tmsp);
        }
        remove_from_file_queue();
    }
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
