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

#include "daemon/dlt_log_channel.h"

#include <thread>

namespace score
{
namespace logging
{
namespace dltserver
{

namespace
{
//  Constant is mean to control number of calls to OS sleep syscalls for DLT file transfer feature.
//  The parameter allows to make actuall sleep call every n-th iteration.
constexpr auto kBurstFileTransferControlCount{5UL};
}  // namespace

void DltLogChannel::sendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                                   uint32_t tmsp,
                                   const void* data,
                                   size_t size)
{
    if (desc.GetLogLevel() > channelThreshold_.load(std::memory_order_relaxed))
    {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    ++non_verbose_.stats_msgcnt;
    ++count_nonverbose_messages_in_buffer_;
    non_verbose_.stats_totalsize += size;
    non_verbose_.message_id_data_stats[desc.GetIdMsgDescriptor()] += size;

    size_t full_size = sizeof(score::platform::internal::DltNvHeaderWithMsgid) + size;
    if (prebuf_is_verbose_)  // flush if DLT type changes
    {
        SendUdp();  // store, flush only if full
        prebuf_is_verbose_ = false;
    }

    if (prebuf_size_ + full_size <= UDP_MAX_PAYLOAD)  // add to current buffer as it fits
    {
        // Use .at() to avoid "non-constant subscript" warning generated from clang
        auto& buffer = prebuf_data_.at(vector_index_);

        // Use iterator arithmetic (not pointer arithmetic) to solve warning generated from clang
        auto it = buffer.begin();
        if (prebuf_size_ > std::numeric_limits<decltype(prebuf_data_)::value_type::difference_type>::max())
        {
            // This check ensures that `prebuf_size_` can be safely converted to a signed type
            // without overflow. However, under normal operating conditions, `prebuf_size_`
            // is constrained by system limits and is derived from predefined buffer sizes or
            // message lengths. Due to these constraints, it is unlikely that this condition
            // will be triggered in practice, making it difficult or impractical to cover in tests.
            // LCOV_EXCL_START
            std::cerr << "prebuf_size_ is too large for signed conversion.\n";
            return;
            // LCOV_EXCL_STOP
        }
        std::advance(it, static_cast<decltype(prebuf_data_)::value_type::difference_type>(prebuf_size_));
        auto* dest = std::addressof(*it);

        /*
            Deviation from Rule M5-2-10:
            - Rule M5-2-10 (required, implementation, automated)
            The increment (++) and decrement ( ) operators shall not be mixed with
            other operators in an expression.
            Justification:
            - Since there are no other operations besides increment, it is quite clear what is happening.
        */
        // coverity[autosar_cpp14_m5_2_10_violation]
        score::platform::internal::ConstructNonVerbosePacket(
            dest, data, size, desc.GetIdMsgDescriptor(), ecu_, mcnt_++, tmsp);

        prebuf_size_ += full_size;
    }
    else  // doesn't fit in current buffer
    {
        SendUdp();
        if (full_size < UDP_MAX_PAYLOAD)  //  fits in our prebuf
        {
            // Use .at() to avoid "non-constant subscript" warning generated from clang
            auto& buffer = prebuf_data_.at(vector_index_);
            auto* dest = buffer.data();

            // coverity[autosar_cpp14_m5_2_10_violation]
            score::platform::internal::ConstructNonVerbosePacket(
                dest, data, size, desc.GetIdMsgDescriptor(), ecu_, mcnt_++, tmsp);

            prebuf_size_ += full_size;
        }
        else  //  single msg is bigger than the prebuf, so prepare it and send using alternative API
        {
            FlushUnprotected();

            static constexpr auto kVectorStackCount = 2UL;
            std::array<iovec, kVectorStackCount> io_vec{};
            score::platform::internal::DltNvHeaderWithMsgid header;
            // coverity[autosar_cpp14_m5_2_10_violation]
            const auto header_size = score::platform::internal::ConstructNonVerboseHeader(
                header, size, desc.GetIdMsgDescriptor(), ecu_, mcnt_++, tmsp);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) iovec is unchangable, It's (POSIX standard)
            io_vec[0].iov_base = static_cast<void*>(&header);
            io_vec[0].iov_len = header_size;
            // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) iovec is unchangable, It's (POSIX standard)
            // const_cast is necessary since data is a const void*
            // coverity[autosar_cpp14_a5_2_3_violation]
            io_vec[1].iov_base = const_cast<void*>(data);
            // NOLINTEND(cppcoreguidelines-pro-type-union-access)
            io_vec[1].iov_len = size;
            const auto send_result = out_.send(io_vec.data(), kVectorStackCount);
            if (send_result.has_value() == false)
            {
                ++non_verbose_.send_failures_count;
                auto& val = non_verbose_.send_errno_count[send_result.error().ToString()];
                ++val;
            }
        }
    }
}

void DltLogChannel::sendVerbose(
    const uint32_t tmsp,
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry)
{
    if (entry.log_level > channelThreshold_.load(std::memory_order_relaxed))
    {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    ++verbose_.stats_msgcnt;
    ++count_verbose_messages_in_buffer_;
    const auto data_size = static_cast<std::uint32_t>(entry.GetPayload().size());

    const size_t full_size = sizeof(score::platform::internal::DltVerboseHeader) + data_size;
    verbose_.stats_totalsize += data_size;

    if (!prebuf_is_verbose_)  //  check and flush if DLT type changes
    {
        SendUdp();
        prebuf_is_verbose_ = true;
    }
    if (prebuf_size_ + full_size <= UDP_MAX_PAYLOAD)
    {
        // Use .at() to avoid "non-constant subscript" warning generated from clang
        auto& buffer = prebuf_data_.at(vector_index_);

        // Use iterator arithmetic (not pointer arithmetic) to solve warning generated from clang
        auto it = buffer.begin();
        if (prebuf_size_ > std::numeric_limits<decltype(prebuf_data_)::value_type::difference_type>::max())
        {
            // This check ensures that `prebuf_size_` can be safely converted to a signed type
            // without overflow. However, under normal operating conditions, `prebuf_size_`
            // is constrained by system limits and is derived from predefined buffer sizes or
            // message lengths. Due to these constraints, it is unlikely that this condition
            // will be triggered in practice, making it difficult or impractical to cover in tests.
            // LCOV_EXCL_START
            std::cerr << "prebuf_size_ is too large for signed conversion.\n";
            return;
            // LCOV_EXCL_STOP
        }
        std::advance(it, static_cast<decltype(prebuf_data_)::value_type::difference_type>(prebuf_size_));
        auto* dest = std::addressof(*it);

        // coverity[autosar_cpp14_m5_2_10_violation]
        score::platform::internal::ConstructVerbosePacket(dest, entry, ecu_, mcnt_++, tmsp);

        prebuf_size_ += full_size;
    }
    else  //  message does not fit into the buffer
    {
        SendUdp();

        if (full_size < UDP_MAX_PAYLOAD)  //  just add it into the buffer if it is less than the buffer size
        {
            // Use .at() to avoid "non-constant subscript" warning generated from clang
            auto& buffer = prebuf_data_.at(vector_index_);
            auto* dest = buffer.data();
            // coverity[autosar_cpp14_m5_2_10_violation]
            score::platform::internal::ConstructVerbosePacket(dest, entry, ecu_, mcnt_++, tmsp);

            prebuf_size_ += full_size;
        }
        else
        {
            FlushUnprotected();

            //  Send big message using alternative API, both for construction and sending:
            static constexpr auto kVectorStackCount = 2UL;
            std::array<iovec, kVectorStackCount> io_vec{};
            score::platform::internal::DltVerboseHeader header;
            // coverity[autosar_cpp14_m5_2_10_violation]
            const auto header_size =
                score::platform::internal::ConstructVerboseHeader(header, entry, ecu_, mcnt_++, tmsp);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) iovec is unchangable, It's (POSIX standard)
            io_vec[0].iov_base = static_cast<void*>(&header);
            io_vec[0].iov_len = header_size;
            // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) iovec is unchangable, It's (POSIX standard)
            // const_cast is necessary since entry.GetPayload().data() is a const void*
            // coverity[autosar_cpp14_a5_2_3_violation]
            io_vec[1].iov_base = const_cast<void*>(static_cast<const void*>(entry.GetPayload().data()));
            // NOLINTEND(cppcoreguidelines-pro-type-union-access)
            io_vec[1].iov_len = static_cast<std::size_t>(entry.GetPayload().size());
            const auto send_result = out_.send(io_vec.data(), kVectorStackCount);
            if (send_result.has_value() == false)
            {
                ++verbose_.send_failures_count;
                auto& val = verbose_.send_errno_count[send_result.error().ToString()];
                ++val;
            }
        }
    }
}

void DltLogChannel::sendFTVerbose(score::cpp::span<const std::uint8_t> data,
                                  const mw::log::LogLevel loglevel,
                                  dltid_t appId,
                                  dltid_t ctxId,
                                  uint8_t nor,
                                  uint32_t tmsp)
{
    static auto start = std::chrono::system_clock::now();
    constexpr auto wait = std::chrono::milliseconds{kBurstFileTransferControlCount};

    static auto iteration_counter = uint32_t{0UL};
    iteration_counter++;
    if (iteration_counter % kBurstFileTransferControlCount == 0UL)
    {
        std::this_thread::sleep_until(start + wait);
    }

    const auto data_size = static_cast<size_t>(data.size());
    score::platform::internal::DltVerboseHeader hdr;
    // coverity[autosar_cpp14_m5_2_10_violation]
    score::platform::internal::ConstructDltStandardHeader(hdr.std, data_size + sizeof(hdr), mcnt_++, true);
    score::platform::internal::ConstructDltStandardHeaderExtra(hdr.stde, ecu_, tmsp);
    score::platform::internal::ConstructDltExtendedHeader(hdr.ext, loglevel, nor, appId, ctxId);

    static constexpr auto kVectorStackCount = 2UL;
    std::array<iovec, kVectorStackCount> io_vec{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) iovec is unchangable, It's (POSIX standard)
    io_vec[0].iov_base = static_cast<void*>(&hdr);
    io_vec[0].iov_len = sizeof(hdr);
    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) iovec is unchangable, It's (POSIX standard)
    // const_cast is necessary since data.data() is a const void*
    // coverity[autosar_cpp14_a5_2_3_violation]
    io_vec[1].iov_base = const_cast<void*>(static_cast<const void*>(data.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
    io_vec[1].iov_len = data_size;

    {  //  lock scope
        std::lock_guard<std::mutex> lock(mutex_);
        FlushUnprotected();
        const auto send_result = out_.send(io_vec.data(), kVectorStackCount);
        if (send_result.has_value() == false)
        {
            ++verbose_.send_failures_count;
            auto& val = verbose_.send_errno_count[send_result.error().ToString()];
            ++val;
        }
        ++verbose_.stats_msgcnt;
        verbose_.stats_totalsize += data_size + sizeof(hdr);
    }
    start = std::chrono::system_clock::now();
}

void DltLogChannel::FlushUnprotected()
{
    SendUdp(true);
}

void DltLogChannel::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    FlushUnprotected();
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
