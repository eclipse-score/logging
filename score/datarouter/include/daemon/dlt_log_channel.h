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

#ifndef DLT_LOG_CHANNEL_H_
#define DLT_LOG_CHANNEL_H_

#include "daemon/dltserver_common.h"
#include "daemon/udp_stream_output.h"
#include "dlt/dlt_headers.h"
#include "score/mw/log/configuration/nvmsgdescriptor.h"
#include "score/mw/log/log_level.h"

#include <score/string_view.hpp>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace score
{
namespace logging
{
namespace dltserver
{

class DltLogChannel
{
  public:
    DltLogChannel(const dltid_t channelName,
                  const mw::log::LogLevel threshold,
                  const dltid_t ecu,
                  const char* srcAddr,
                  const uint16_t srcPort,
                  const char* dstAddr,
                  const uint16_t dstPort,
                  const char* multicastInterface)
        : channelName_(channelName),
          ecu_(ecu),
          channelThreshold_(threshold),
          mutex_{},
          out_(dstAddr, dstPort, multicastInterface),
          mcnt_(0),
          count_verbose_messages_in_buffer_(0),
          count_nonverbose_messages_in_buffer_(0),
          prebuf_data_{},
          io_vec_{},
          mmsg_hdr_array_{},
          prebuf_size_(0),
          prebuf_is_verbose_(false),
          srcport_(srcPort),
          bind_result_{},
          verbose_(),
          non_verbose_()
    {
        bind_result_ = out_.bind(srcAddr, srcPort);
    }

    DltLogChannel(const std::string& channelName,
                  const mw::log::LogLevel threshold,
                  const std::string& ecu,
                  const char* srcAddr,
                  const uint16_t srcPort,
                  const char* dstAddr,
                  const uint16_t dstPort,
                  const char* multicastInterface)
        : channelName_(dltid_t(channelName)),
          ecu_(dltid_t(ecu)),
          channelThreshold_(threshold),
          mutex_{},
          out_(dstAddr, dstPort, multicastInterface),
          mcnt_(0),
          count_verbose_messages_in_buffer_(0),
          count_nonverbose_messages_in_buffer_(0),
          prebuf_data_{},
          io_vec_{},
          mmsg_hdr_array_{},
          prebuf_size_(0),
          prebuf_is_verbose_(false),
          srcport_(srcPort),
          bind_result_{},
          verbose_(),
          non_verbose_()
    {
        bind_result_ = out_.bind(srcAddr, srcPort);
    }

    ~DltLogChannel() = default;
    DltLogChannel(const DltLogChannel&) = delete;
    DltLogChannel(DltLogChannel&& from) noexcept
        : channelName_(from.channelName_),
          ecu_(from.ecu_),
          channelThreshold_(from.channelThreshold_.load()),
          mutex_{},
          out_(std::move(from.out_)),
          mcnt_(0),
          count_verbose_messages_in_buffer_(0),
          count_nonverbose_messages_in_buffer_(0),
          prebuf_data_{},
          io_vec_{},
          mmsg_hdr_array_{},
          prebuf_size_(0),
          prebuf_is_verbose_(false),
          srcport_(from.srcport_),
          bind_result_(from.bind_result_),
          verbose_(),
          non_verbose_()
    {
    }

    void sendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                        uint32_t tmsp,
                        const void* data,
                        size_t size);
    void sendVerbose(const uint32_t tmsp,
                     const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry);
    //  FT stands for 'file transfer'
    void sendFTVerbose(score::cpp::span<const std::uint8_t> data,
                       const mw::log::LogLevel loglevel,
                       dltid_t appId,
                       dltid_t ctxId,
                       uint8_t nor,
                       uint32_t tmsp);

    template <typename Logger>
    void show_stats(Logger& stat_logger)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ShowAndClearStatsDlt(verbose_, stat_logger, channelName_);
        ShowAndClearStatsNonVerbose(non_verbose_, stat_logger, channelName_);
    }

    void flush();

    const dltid_t channelName_;
    const dltid_t ecu_;

    std::atomic<mw::log::LogLevel> channelThreshold_;

  private:
    static constexpr uint32_t kIpv4HeaderWithoutOptions = 20UL;
    static constexpr uint32_t kUdpHeader = 8UL;
    static constexpr uint32_t kMtu = 1500UL;
    static constexpr uint32_t UDP_MAX_PAYLOAD = kMtu - (kIpv4HeaderWithoutOptions + kUdpHeader);
    static constexpr uint16_t bandwidth_denominator_ = 10 /* show_stats() cycle_time */ * 1024 /* KiBytes */;

    std::mutex mutex_;
    UdpStreamOutput out_;
    uint8_t mcnt_;
    uint8_t count_verbose_messages_in_buffer_;
    uint8_t count_nonverbose_messages_in_buffer_;
    static constexpr auto kVectorCount = 4UL;
    uint32_t vector_index_ = 0UL;
    std::array<std::array<char, UDP_MAX_PAYLOAD>, kVectorCount> prebuf_data_;
    std::array<iovec, kVectorCount> io_vec_;
    std::array<mmsghdr, kVectorCount> mmsg_hdr_array_;
    size_t prebuf_size_;
    bool prebuf_is_verbose_;
    // Statistics variables
    int srcport_;
    score::cpp::expected_blank<score::os::Error> bind_result_;  // Save result for later error reporting

    void FlushAndSendVerboseUnprotected(void* msg, size_t msg_size);

    struct DltLogChannelStatistics
    {
        uint64_t stats_msgcnt{};
        uint64_t stats_totalsize{};
        uint64_t send_failures_count{};
        std::unordered_map<std::string, std::uint64_t> send_errno_count{};
    };
    DltLogChannelStatistics verbose_;

    void FlushUnprotected();

    inline void SendUdp(bool flush = false)
    {
        if (prebuf_size_ > 0)
        {
            // array.at() won't throw the exception that we do boundary check below
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) cannot change due to qnx struct
            io_vec_.at(vector_index_).iov_base = prebuf_data_.at(vector_index_).data();
            io_vec_.at(vector_index_).iov_len = prebuf_size_;

            mmsg_hdr_array_.at(vector_index_).msg_hdr.msg_iov = &io_vec_.at(vector_index_);
            mmsg_hdr_array_.at(vector_index_).msg_hdr.msg_iovlen = 1;

            vector_index_++;
            prebuf_size_ = 0;
        }

        if ((flush && vector_index_ > 0) || vector_index_ >= kVectorCount)
        {
            score::cpp::span<mmsghdr> mmsg_span(mmsg_hdr_array_.data(), vector_index_);
            const auto send_result = out_.send(mmsg_span);
            if (send_result.has_value() == false)
            {
                if (count_verbose_messages_in_buffer_ > 0)
                {
                    ++verbose_.send_failures_count;
                    /*
                        Deviation from Rule M5-2-10:
                        - Rule M5-2-10 (required, implementation, automated)
                        The increment (++) and decrement ( ) operators shall not be mixed with
                        other operators in an expression.
                        Justification:
                        - Since there are no other operations besides increment, it is quite clear what is happening.
                    */
                    // coverity[autosar_cpp14_m5_2_10_violation]
                    ++verbose_.send_errno_count[send_result.error().ToString()];
                }
                if (count_nonverbose_messages_in_buffer_ > 0)
                {
                    ++non_verbose_.send_failures_count;
                    // coverity[autosar_cpp14_m5_2_10_violation] see above.
                    ++non_verbose_.send_errno_count[send_result.error().ToString()];
                }
            }
            vector_index_ = 0;
            count_verbose_messages_in_buffer_ = 0;
            count_nonverbose_messages_in_buffer_ = 0;
        }
    }

    struct DltLogChannelNonVerboseStatistics : public DltLogChannelStatistics
    {
        std::unordered_map<uint32_t, size_t> message_id_data_stats;
    };
    DltLogChannelNonVerboseStatistics non_verbose_;

    template <typename Logger>
    void ShowAndClearStatsDlt(DltLogChannelStatistics& statistics,
                              Logger& stat_logger,
                              dltid_t channel_name,
                              const score::cpp::string_view statistics_type = "verbose")
    {
        auto log_stream{stat_logger.LogInfo()};
        log_stream << std::string(statistics_type.data(), statistics_type.size())
                   << " messages in the channel:" << std::string{channel_name.data(), channel_name.size()} << ": count "
                   << statistics.stats_msgcnt << ", size " << statistics.stats_totalsize << " bytes ("
                   << statistics.stats_totalsize / bandwidth_denominator_ << " kiB/s)"
                   << "failed to send: total count " << statistics.send_failures_count;

        if (statistics.send_failures_count > 0)
        {
            for (const auto& error_item : statistics.send_errno_count)
            {
                log_stream << ", failed to send " << error_item.second << " times due to \"" << error_item.first
                           << "\"";
            }
        }
        if (bind_result_.has_value() == false)
        {
            log_stream << ", failed to bind: " << bind_result_.error().ToString();
        }

        //  Cleanup:
        statistics.stats_msgcnt = 0;
        statistics.stats_totalsize = 0;
        statistics.send_failures_count = 0;
        return;
    }

    template <typename Logger>
    void ShowAndClearStatsNonVerbose(DltLogChannelNonVerboseStatistics& statistics,
                                     Logger& stat_logger,
                                     dltid_t channel_name)
    {
        ShowAndClearStatsDlt(statistics, stat_logger, channel_name, "non-verbose");

        std::vector<std::pair<uint32_t, size_t>> dlt_non_verbose_diagnostics(begin(statistics.message_id_data_stats),
                                                                             end(statistics.message_id_data_stats));
        std::sort(begin(dlt_non_verbose_diagnostics),
                  end(dlt_non_verbose_diagnostics),
                  [](const auto& elem1, const auto& elem2) {
                      return elem1.second > elem2.second;
                  });

        auto&& log_stream = stat_logger.LogInfo();
        log_stream << "dlt stats: non-verbose messages in channel:"
                   << std::string{channel_name.data(), channel_name.size()} << " sent data by message id.";
        std::for_each(
            begin(dlt_non_verbose_diagnostics), end(dlt_non_verbose_diagnostics), [&log_stream](const auto& elem) {
                log_stream << "Msgid:" << elem.first << " bytes:" << elem.second << " ("
                           << elem.second / bandwidth_denominator_ << "kiB/s) |";
            });

        //  Cleanup:
        statistics.message_id_data_stats.clear();
        return;
    }
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // DLT_LOG_CHANNEL_H_
