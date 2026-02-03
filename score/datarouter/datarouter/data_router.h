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

#ifndef DATA_ROUTER_H_
#define DATA_ROUTER_H_

#include "daemon/message_passing_server.h"
#include "logparser/logparser.h"
#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/detail/data_router/shared_memory/reader_factory.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"
#include "score/datarouter/daemon_communication/session_handle_interface.h"
#include "unix_domain/unix_domain_server.h"

#include "score/mw/log/logger.h"
#include "score/datarouter/lib/synchronized/synchronized.h"

#include "score/variant.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace score
{
namespace platform
{
namespace datarouter
{

using internal::ILogParser;
using internal::MessagePassingServer;
using internal::UnixDomainServer;

std::string QuotaValueAsString(double quota) noexcept;

struct LocalSubscriberData
{
    std::chrono::microseconds time_between_to_calls{std::chrono::microseconds::zero()};
    std::chrono::microseconds time_to_process_records{std::chrono::microseconds::zero()};
    std::chrono::steady_clock::time_point last_call_timestamp{std::chrono::steady_clock::now()};
    bool detach_on_closed_processed{false};
    bool enabled_logging_at_server_{false};
};

struct CommandData
{
    bool command_detach_on_closed{false};
    bool acquire_requested{false};
    uint8_t ticks_without_write{0};
    std::optional<std::uint32_t> block_expected_to_be_next{std::nullopt};
    std::optional<score::mw::log::detail::ReadAcquireResult> data_acquired_{std::nullopt};
};

struct StatsData
{
    uint64_t message_count{0};
    uint64_t message_count_dropped{0};
    uint64_t size_dropped{0};
    uint64_t message_count_dropped_invalid_size{0};
    uint64_t max_bytes_in_buffer{0};
    uint64_t totalsize{0};
    double quota_KBps{0.0};
    bool quota_enforcement_enabled{false};
    bool quota_overlimit_detected{false};
    std::chrono::microseconds time_spent_reading{std::chrono::microseconds::zero()};
    std::chrono::microseconds transport_delay{std::chrono::microseconds::zero()};
    std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
    std::string name;
    uint64_t count_acquire_requests{0};
};

class DataRouter
{
  public:
    using SourceSetupCallback = std::function<void(ILogParser&&)>;
    using SessionPtr = std::unique_ptr<UnixDomainServer::ISession>;
    using MessagingSessionPtr = std::unique_ptr<MessagePassingServer::ISession>;

    using SessionHandleVariant = score::cpp::variant<UnixDomainServer::SessionHandle,
                                              score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle>>;

    explicit DataRouter(score::mw::log::Logger& logger, SourceSetupCallback sourceCallback = SourceSetupCallback());

    MessagingSessionPtr new_source_session(
        int fd,
        const std::string name,
        const bool is_dlt_enabled,
        score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle,
        const double quota,
        bool quota_enforcement_enabled,
        const pid_t client_pid,
        const score::mw::log::NvConfig& nvConfig,
        score::mw::log::detail::ReaderFactoryPtr reader_factory =
            score::mw::log::detail::ReaderFactory::Default(score::cpp::pmr::get_default_resource()));

    template <typename E, typename F>
    void for_each_source_parser(E e, F f, bool enable_logging_client)
    {
        std::lock_guard<std::mutex> lock(subscriber_mutex_);
        for (auto& sourceSession : sources_)
        {
            // No need for the extra lock - synchronization is handled by the Synchronized<T> wrapper
            sourceSession->SetLoggingClientEnabled(enable_logging_client);
            e(sourceSession->get_parser());
        }
        f();
    }

    void show_source_statistics(uint16_t series_num);

    // for unit test only. to keep rest of functions in private
    class DataRouterForTest;

  private:
    /**
     * class SourceSession is private to Datarouter and could not be used outside Datarouter, so it is safe to keep in
     * public visibility everything used by Datarouter directly, because outside Datarouter it is visible as
     * MessagePassingServer::ISession / UnixDomainServer::ISession
     */
    // NOLINTNEXTLINE(fuchsia-multiple-inheritance) - Both base classes are pure interfaces
    class SourceSession : public UnixDomainServer::ISession, public MessagePassingServer::ISession
    {
      public:
        SourceSession(DataRouter& router,
                      std::unique_ptr<score::mw::log::detail::ISharedMemoryReader> reader,
                      const std::string& name,
                      const bool is_dlt_enabled,
                      SessionHandleVariant handle,
                      const double quota,
                      bool quota_enforcement_enabled,
                      score::mw::log::Logger& stats_logger,
                      std::unique_ptr<score::platform::internal::ILogParser> parser);

        SourceSession(const SourceSession&) = delete;
        SourceSession& operator=(const SourceSession&) = delete;
        SourceSession(SourceSession&&) = delete;
        SourceSession& operator=(SourceSession&&) = delete;

        void SetLoggingClientEnabled(bool enable)
        {
            local_subscriber_data_.lock()->enabled_logging_at_server_ = enable;
        }
        ~SourceSession();

        // for unit test only. to keep rest of functions in private
        class SourceSessionForTest;

      private:
        bool is_source_closed() override
        {
            return local_subscriber_data_.lock()->detach_on_closed_processed;
        }

        bool tick() override;

        bool tryFinalizeAcquisition(bool& needs_fast_reschedule);
        void processAndRouteLogMessages(uint64_t& message_count_local,
                                        std::chrono::microseconds& transport_delay_local,
                                        uint64_t& number_of_bytes_in_buffer,
                                        bool acquire_finalized_in_this_tick,
                                        bool& needs_fast_reschedule);
        void update_and_log_stats(uint64_t message_count_local,
                                  uint64_t number_of_bytes_in_buffer,
                                  std::chrono::microseconds transport_delay_local,
                                  score::os::HighResolutionSteadyClock::time_point start);
        void process_detached_logs(uint64_t& number_of_bytes_in_buffer);

        void on_acquire_response(const score::mw::log::detail::ReadAcquireResult& acq) override;

        void on_closed_by_peer() override;

        void checkAndSetQuotaEnforcement();
        void request_acquire();

        Synchronized<LocalSubscriberData> local_subscriber_data_;
        Synchronized<CommandData> command_data_;
        Synchronized<StatsData> stats_data_;

        DataRouter& router_;
        std::unique_ptr<score::mw::log::detail::ISharedMemoryReader> reader_;
        std::unique_ptr<score::platform::internal::ILogParser> parser_;
        SessionHandleVariant handle_;
        score::mw::log::Logger& stats_logger_;

      public:
        void show_stats();
        ILogParser& get_parser()
        {
            return *(parser_.get());
        }
    };

    std::unique_ptr<SourceSession> new_source_session_impl(
        const std::string name,
        const bool is_dlt_enabled,
        SessionHandleVariant handle,
        const double quota,
        bool quota_enforcement_enabled,
        std::unique_ptr<score::mw::log::detail::ISharedMemoryReader> reader,
        const score::mw::log::NvConfig& nvConfig);

    score::mw::log::Logger& stats_logger_;

    std::unordered_set<SourceSession*> sources_;
    SourceSetupCallback sourceCallback_;

    std::mutex subscriber_mutex_;
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // DATA_ROUTER_H_
