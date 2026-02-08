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

#include "score/datarouter/datarouter/data_router.h"

#include "score/mw/log/logging.h"

#include "score/os/unistd.h"

#include "score/overload.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace score
{
namespace platform
{
namespace datarouter
{

namespace
{
/// The kTicksWithoutAcquireWhileNoWrites parameter is regulating the minimal frequency of polling the logging client
/// with Read Acquire Requests. The minimal threshold was reached to still have the keepalive channel for the Client.
/// This measure is taken to prevent shared memory object leaking i.e. keeping allocated shared memory resources of the
/// client that already have died. There should be a feedback information about existence of the peer when we try to
/// send a message to the client. Sends Read Acquire Request even if the client buffer is empty with at least once per
/// kTicksWithoutAcquireWhileNoWrites polling intervals.
constexpr std::uint8_t kTicksWithoutAcquireWhileNoWrites = 10UL;

template <typename T>
score::mw::log::LogStream& operator<<(score::mw::log::LogStream& log_stream, const std::optional<T>& data) noexcept
{
    if (data.has_value())
    {
        log_stream << "[" << data.value() << "]";
    }
    else
    {
        log_stream << "[std::nullopt]";
    }
    return log_stream;
}
}  //  namespace

// We can't test such function because it's a free function inside anonymous namespace that inside cpp file.
// So, we suppressed the un-covered lines.
std::string QuotaValueAsString(double quota) noexcept
{
    std::stringstream ss;

    // use >= in comparison, because comparing floating point with == or != is unsafe
    if (quota >= std::numeric_limits<decltype(quota)>::max())
    {
        ss << "[unlimited]";
    }
    else
    {
        ss << quota;
    }

    return ss.str();
}

DataRouter::DataRouter(score::mw::log::Logger& logger, SourceSetupCallback sourceCallback)
    : stats_logger_(logger), sourceCallback_(sourceCallback)
{
}

DataRouter::MessagingSessionPtr DataRouter::new_source_session(
    int fd,
    const std::string name,
    const bool is_dlt_enabled,
    score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle,
    const double quota,
    bool quota_enforcement_enabled,
    const pid_t client_pid,
    const score::mw::log::NvConfig& nvConfig,
    score::mw::log::detail::ReaderFactoryPtr reader_factory)
{
    //  It shall be safe to create shared memory reader as only single process - Datarouter daemon, shall be running
    //  at all times in whole system.
    auto reader = reader_factory->Create(fd, client_pid);
    if (reader == nullptr)
    {
        stats_logger_.LogError() << "Failed to create session for pid=" << client_pid << ", appid=" << name;
        return nullptr;
    }

    return new_source_session_impl(
        name, is_dlt_enabled, std::move(handle), quota, quota_enforcement_enabled, std::move(reader), nvConfig);
}

void DataRouter::show_source_statistics(uint16_t series_num)
{
    std::lock_guard<std::mutex> lock(subscriber_mutex_);
    stats_logger_.LogInfo() << "log stat #" << series_num;
    for (auto& sourceSession : sources_)
    {
        sourceSession->show_stats();
    }
}

std::unique_ptr<DataRouter::SourceSession> DataRouter::new_source_session_impl(
    const std::string name,
    const bool is_dlt_enabled,
    SessionHandleVariant handle,
    const double quota,
    bool quota_enforcement_enabled,
    std::unique_ptr<score::mw::log::detail::ISharedMemoryReader> reader,
    const score::mw::log::NvConfig& nvConfig)
{
    auto sourceSession =
        std::make_unique<DataRouter::SourceSession>(*this,
                                                    std::move(reader),
                                                    name,
                                                    is_dlt_enabled,
                                                    std::move(handle),
                                                    quota,
                                                    quota_enforcement_enabled,
                                                    stats_logger_,
                                                    std::make_unique<score::platform::internal::LogParser>(nvConfig));

    if (!sourceSession)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(subscriber_mutex_);

    // Insert is protected by subscriber_mutex_ held by caller (new_source_session_impl).
    // This relies on the calling convention that SourceSession is only constructed
    // from new_source_session_impl() which acquires the lock before construction.
    std::ignore = sources_.insert(sourceSession.get());

    if (sourceCallback_)
    {
        sourceCallback_(std::move(sourceSession->get_parser()));
    }
    // persistent subscribers
    return sourceSession;
}

bool DataRouter::SourceSession::tick()
{
    if (local_subscriber_data_.lock()->detach_on_closed_processed)
    {
        return false;
    }

    // Phase 1: finalize a pending acquire if possible
    bool needs_fast_reschedule{false};
    const bool acquire_finalized = tryFinalizeAcquisition(needs_fast_reschedule);

    uint64_t message_count_local = 0;
    uint64_t number_of_bytes_in_buffer = 0;
    std::chrono::microseconds transport_delay_local = std::chrono::microseconds::zero();
    auto start = score::os::HighResolutionSteadyClock::now();

    processAndRouteLogMessages(message_count_local,
                               transport_delay_local,
                               number_of_bytes_in_buffer,
                               acquire_finalized,
                               needs_fast_reschedule);

    update_and_log_stats(message_count_local, number_of_bytes_in_buffer, transport_delay_local, start);

    // NOTE: keep historical external API: tick() returns false.
    // Scheduler/tests rely on this; internal reschedule hint is tracked via needs_fast_reschedule.
    return false;
}

bool DataRouter::SourceSession::tryFinalizeAcquisition(bool& needs_fast_reschedule)
{
    std::optional<score::mw::log::detail::ReadAcquireResult> data_acquired_local;

    data_acquired_local = command_data_.lock()->data_acquired_;

    if (data_acquired_local.has_value())
    {
        if (reader_->IsBlockReleasedByWriters(data_acquired_local.value().acquired_buffer))
        {
            std::ignore = reader_->NotifyAcquisitionSetReader(data_acquired_local.value());
            command_data_.lock()->data_acquired_ = std::nullopt;

            return true;
        }
        else
        {
            needs_fast_reschedule = true;
        }
    }

    return false;
}

void DataRouter::SourceSession::processAndRouteLogMessages(uint64_t& message_count_local,
                                                           std::chrono::microseconds& transport_delay_local,
                                                           uint64_t& number_of_bytes_in_buffer,
                                                           bool acquire_finalized_in_this_tick,
                                                           bool& needs_fast_reschedule)
{
    const auto current_timestamp = std::chrono::steady_clock::now();

    {
        auto locked_data = local_subscriber_data_.lock();
        locked_data->time_between_to_calls =
            std::chrono::duration_cast<std::chrono::microseconds>(current_timestamp - locked_data->last_call_timestamp);
        locked_data->last_call_timestamp = current_timestamp;
    }

    bool quota_limit_exceeded = stats_data_.lock()->quota_overlimit_detected;

    score::mw::log::detail::TypeRegistrationCallback on_new_type =
        [this](const score::mw::log::detail::TypeRegistration& registration) noexcept {
            parser_->AddIncomingType(registration);
        };

    score::mw::log::detail::NewRecordCallback on_new_record =
        [this, &message_count_local, &transport_delay_local, quota_limit_exceeded](
            const score::mw::log::detail::SharedMemoryRecord& record) noexcept {
            if (quota_limit_exceeded)
            {
                return;
            }

            auto record_received_timestamp = score::mw::log::detail::TimePoint::clock::now();
            parser_->Parse(record);
            ++message_count_local;

            transport_delay_local = std::max(transport_delay_local,
                                             std::chrono::duration_cast<std::chrono::microseconds>(
                                                 record_received_timestamp - record.header.time_stamp));
        };

    bool detach_needed = false;
    const auto number_of_bytes_in_buffer_result = reader_->Read(on_new_type, on_new_record);
    if (number_of_bytes_in_buffer_result.has_value())
    {
        number_of_bytes_in_buffer = number_of_bytes_in_buffer_result.value();
    }

    detach_needed = command_data_.lock()->command_detach_on_closed;

    if (detach_needed)
    {
        local_subscriber_data_.lock()->detach_on_closed_processed = true;
        process_detached_logs(number_of_bytes_in_buffer);
    }

    bool enabled_logging = local_subscriber_data_.lock()->enabled_logging_at_server_;

    {
        auto cmd = command_data_.lock();
        if (acquire_finalized_in_this_tick)
        {
            cmd->acquire_requested = false;
            cmd->ticks_without_write = 0;
        }
        else if (!cmd->acquire_requested && enabled_logging && !detach_needed)
        {
            if (cmd->block_expected_to_be_next.has_value())
            {
                const auto peek_bytes =
                    reader_->PeekNumberOfBytesAcquiredInBuffer(cmd->block_expected_to_be_next.value());

                if ((peek_bytes.has_value() && peek_bytes.value() > 0) ||
                    (cmd->ticks_without_write > kTicksWithoutAcquireWhileNoWrites))
                {
                    cmd->acquire_requested = request_acquire();
                    needs_fast_reschedule = cmd->acquire_requested;
                }
                else
                {
                    auto& ticks = cmd->ticks_without_write;
                    ++ticks;
                }
            }
            else
            {
                cmd->acquire_requested = request_acquire();
                needs_fast_reschedule = cmd->acquire_requested;
            }
        }
    }

    local_subscriber_data_.lock()->time_to_process_records =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - current_timestamp);
}

void DataRouter::SourceSession::process_detached_logs(uint64_t& number_of_bytes_in_buffer)
{
    const auto number_of_bytes_in_buffer_result_detached = reader_->ReadDetached(
        [this](const auto& registration) noexcept {
            parser_->AddIncomingType(registration);
        },
        [this](const auto& record) noexcept {
            parser_->Parse(record);
        });

    if (number_of_bytes_in_buffer_result_detached.has_value())
    {
        number_of_bytes_in_buffer = number_of_bytes_in_buffer_result_detached.value();
    }

    std::string name = stats_data_.lock()->name;
    stats_logger_.LogError() << name << ": detached logs processed: " << number_of_bytes_in_buffer_result_detached;
}

void DataRouter::SourceSession::update_and_log_stats(uint64_t message_count_local,
                                                     uint64_t number_of_bytes_in_buffer,
                                                     std::chrono::microseconds transport_delay_local,
                                                     score::os::HighResolutionSteadyClock::time_point start)
{
    {
        auto stats = stats_data_.lock();

        const auto message_count_dropped_new = reader_->GetNumberOfDropsWithBufferFull();
        const auto size_dropped_new = reader_->GetSizeOfDropsWithBufferFull();
        if (message_count_dropped_new != stats->message_count_dropped)
        {
            stats_logger_.LogError() << stats->name << ": message drop detected: "
                                     << message_count_dropped_new - stats->message_count_dropped << " messages, "
                                     << size_dropped_new - stats->size_dropped << " bytes lost due to buffer full!";
            stats->message_count_dropped = message_count_dropped_new;
            stats->size_dropped = size_dropped_new;
        }

        const auto message_count_dropped_invalid_size_new = reader_->GetNumberOfDropsWithInvalidSize();
        if (message_count_dropped_invalid_size_new != stats->message_count_dropped_invalid_size)
        {
            stats_logger_.LogError() << stats->name << ": message drop detected: "
                                     << message_count_dropped_invalid_size_new -
                                            stats->message_count_dropped_invalid_size
                                     << " messages lost due to invalid size!";
            stats->message_count_dropped_invalid_size = message_count_dropped_invalid_size_new;
        }

        stats->message_count += message_count_local;
        stats->totalsize += number_of_bytes_in_buffer;
        stats->max_bytes_in_buffer = std::max(stats->max_bytes_in_buffer, number_of_bytes_in_buffer);
        stats->transport_delay = std::max(stats->transport_delay, transport_delay_local);
        stats->time_spent_reading +=
            std::chrono::duration_cast<std::chrono::microseconds>(score::platform::timestamp_t::clock::now() - start);
    }

    checkAndSetQuotaEnforcement();
}

DataRouter::SourceSession::SourceSession(DataRouter& router,
                                         std::unique_ptr<score::mw::log::detail::ISharedMemoryReader> reader,
                                         const std::string& name,
                                         const bool is_dlt_enabled,
                                         SessionHandleVariant handle,
                                         const double quota,
                                         bool quota_enforcement_enabled,
                                         score::mw::log::Logger& stats_logger,
                                         std::unique_ptr<score::platform::internal::ILogParser> parser)
    : UnixDomainServer::ISession{},
      MessagePassingServer::ISession{},
      local_subscriber_data_(LocalSubscriberData{}),
      command_data_(CommandData{}),
      stats_data_(StatsData{}),
      router_(router),
      reader_(std::move(reader)),
      parser_(std::move(parser)),
      handle_(std::move(handle)),
      stats_logger_(stats_logger)
{
    local_subscriber_data_.lock()->enabled_logging_at_server_ = is_dlt_enabled;
    {
        auto stats = stats_data_.lock();
        stats->quota_KBps = quota;
        stats->quota_enforcement_enabled = quota_enforcement_enabled;
        stats->name = name;
    }

    {
        auto stats = stats_data_.lock();
        if (stats->name == "DR")
        {
            constexpr double newQuotaValue = std::numeric_limits<double>::max();
            stats_logger_.LogInfo() << "Override quota value for Datarouter (to be unlimited). Old value: "
                                    << QuotaValueAsString(stats->quota_KBps)
                                    << ", new value: " << QuotaValueAsString(newQuotaValue);
            stats->quota_KBps = newQuotaValue;
        }
    }
}

DataRouter::SourceSession::~SourceSession()
{
    {
        std::lock_guard<std::mutex> lock(router_.subscriber_mutex_);
        std::ignore = router_.sources_.erase(this);
    }

    stats_logger_.LogInfo() << "Cleaning up source session for " << stats_data_.lock()->name;
}

void DataRouter::SourceSession::checkAndSetQuotaEnforcement()
{
    auto stats = stats_data_.lock();
    if (!stats->quota_overlimit_detected && stats->quota_enforcement_enabled)
    {
        const auto time_now = std::chrono::steady_clock::now();
        const auto tstat_in_msec =
            std::chrono::duration_cast<std::chrono::milliseconds>(time_now - stats->start).count();

        if (tstat_in_msec == 0)
        {
            stats_logger_.LogError()
                << stats->name
                << ": time duration is 0. Data rate could not be calculated. Quota enforcement is not applied.";
            return;
        }

        const auto rate_KBps =
            static_cast<double>(stats->totalsize) * 1000. / 1024. / static_cast<double>(tstat_in_msec);

        stats_logger_.LogInfo() << stats->name << "quota status. rate: " << rate_KBps
                                << ", quota_KBps_: " << QuotaValueAsString(stats->quota_KBps)
                                << ", totalsize_: " << stats->totalsize << ", tstat_in_msec: " << tstat_in_msec;

        if (rate_KBps > stats->quota_KBps)
        {
            stats_logger_.LogError() << stats->name
                                     << ": exceeded the quota. quota enforcement set. rate: " << rate_KBps
                                     << ", quota_KBps: " << QuotaValueAsString(stats->quota_KBps);
            stats->quota_overlimit_detected = true;
        }
    }
}

void DataRouter::SourceSession::show_stats()
{
    uint64_t message_count{0};
    uint64_t totalsize{0};
    double quota_KBps{0.0};
    bool quota_enforcement_enabled{false};
    bool quota_overlimit_detected{false};
    std::chrono::microseconds time_spent_reading{};
    std::chrono::microseconds transport_delay{};
    uint64_t message_count_dropped{0};
    uint64_t count_acquire_requests{0};
    uint64_t max_bytes_in_buffer{0};
    std::string name;

    {
        auto stats = stats_data_.lock();
        message_count = stats->message_count;
        totalsize = stats->totalsize;
        quota_KBps = stats->quota_KBps;
        quota_enforcement_enabled = stats->quota_enforcement_enabled;
        quota_overlimit_detected = stats->quota_overlimit_detected;
        time_spent_reading = stats->time_spent_reading;
        transport_delay = stats->transport_delay;
        message_count_dropped = stats->message_count_dropped;
        count_acquire_requests = stats->count_acquire_requests;
        max_bytes_in_buffer = stats->max_bytes_in_buffer;
        name = stats->name;
    }

    const auto buffer_size_kb = reader_->GetRingBufferSizeBytes() / 1024U / 2U;
    auto buffer_watermark_kb = max_bytes_in_buffer / 1024U;

    if (message_count_dropped > 0)
    {
        buffer_watermark_kb = buffer_size_kb;
    }

    const auto buffer_watermark_percent =
        (buffer_size_kb != 0U) ? std::to_string((100U * buffer_watermark_kb) / buffer_size_kb) : std::string{"n.a."};

    auto last_start = std::chrono::steady_clock::time_point{};
    auto current_time = std::chrono::steady_clock::now();
    {
        auto stats = stats_data_.lock();
        last_start = stats->start;
        stats->start = current_time;
        stats->message_count = 0;
        stats->totalsize = 0;
        stats->time_spent_reading = std::chrono::microseconds::zero();
        stats->transport_delay = std::chrono::microseconds::zero();
        if (stats->quota_overlimit_detected)
        {
            stats->quota_overlimit_detected = false;
        }
    }

    auto tstat_in_msec = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_start);
    auto rate_KBps = static_cast<double>(totalsize) * 1000. / 1024. / static_cast<double>(tstat_in_msec.count());

    auto [time_between_calls, time_to_process] = [this]() noexcept {
        auto subs_data = local_subscriber_data_.lock();
        return std::make_pair(subs_data->time_between_to_calls.count(), subs_data->time_to_process_records.count());
    }();

    stats_logger_.LogInfo() << name << ": count " << message_count << ", size " << totalsize
                            << " B, rate: " << rate_KBps << " KBps"
                            << ", quota rate: " << QuotaValueAsString(quota_KBps)
                            << ", quota enforcement: " << quota_overlimit_detected
                            << ", read_time:" << time_spent_reading.count() << " us"
                            << ", transp_delay:" << transport_delay.count() << " us"
                            << ", time_between_to_calls_us:" << time_between_calls << " us"
                            << ", time_to_process_records_:" << time_to_process << " us"
                            << ", buffer size watermark: " << buffer_watermark_kb << " KB out of" << buffer_size_kb
                            << " KB (" << buffer_watermark_percent << "%)"
                            << ", messages dropped: " << message_count_dropped << " (accumulated)"
                            << ", IPC count: " << count_acquire_requests;

    if (rate_KBps > quota_KBps && quota_enforcement_enabled)
    {
        stats_logger_.LogError() << name << ": exceeded the quota of " << QuotaValueAsString(quota_KBps)
                                 << "KBps, rate " << rate_KBps << " KBps";
    }
    if (quota_overlimit_detected)
    {
        stats_logger_.LogInfo() << name << ": clear quota enforcement";
    }
}

bool DataRouter::SourceSession::request_acquire()
{
    const bool acquire_result =
        score::cpp::visit(score::cpp::overload(
                       [](UnixDomainServer::SessionHandle& handle) {
                           handle.pass_message("<");
                           return true;
                       },
                       [](score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle>& handle) {
                           return handle->AcquireRequest();
                           // For the quality team argumentation, kindly, check Ticket-200702 and Ticket-229594.
                       }),  // LCOV_EXCL_LINE : tooling issue. no code to test in this line.
                   handle_);

    if (acquire_result)
    {
        auto stats = stats_data_.lock();
        auto& count_ref = stats->count_acquire_requests;
        ++count_ref;
    }

    return acquire_result;
}

void DataRouter::SourceSession::on_acquire_response(const score::mw::log::detail::ReadAcquireResult& acq)
{
    auto cmd = command_data_.lock();
    cmd->data_acquired_ = acq;
    cmd->block_expected_to_be_next = GetExpectedNextAcquiredBlockId(acq);
}

void DataRouter::SourceSession::on_closed_by_peer()
{
    command_data_.lock()->command_detach_on_closed = true;
}

}  // namespace datarouter
}  // namespace platform
}  // namespace score
