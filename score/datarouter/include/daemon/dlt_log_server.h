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

#ifndef SCORE_DATAROUTER_INCLUDE_DAEMON_DLT_LOG_SERVER_H
#define SCORE_DATAROUTER_INCLUDE_DAEMON_DLT_LOG_SERVER_H

#include "applications/datarouter_feature_config.h"
#include "daemon/diagnostic_job_handler.h"
#include "daemon/diagnostic_job_parser.h"
#include "daemon/dlt_log_server_config.h"
#include "daemon/verbose_dlt.h"
#include "i_session.h"
#include "logparser/logparser.h"
#include "unix_domain/unix_domain_server.h"

#include "score/span.hpp"

#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/log_level.h"
#include "score/datarouter/include/daemon/log_sender.h"

#include <atomic>
#include <bitset>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

// Needed by datarouter feature config types.
using namespace score::platform::datarouter;

namespace score
{
namespace logging
{
namespace dltserver
{

const std::string kLogEntryTypeName{"score::mw::log::detail::LogEntry"};
const std::string kPersistentRequestTypeName{"score::logging::internal::PersistentLoggingRequestStructure"};
const std::string kFileTransferTypeName{"score::logging::FileTransferEntry"};
class DltLogServer : score::platform::datarouter::DltNonverboseHandlerType::IOutput,
                     DltVerboseHandler::IOutput,
                     FileTransferStreamHandlerType::IOutput,
                     IDltLogServer
{
  public:
    using SessionPtr = std::unique_ptr<score::logging::ISession>;
    using EnabledCallback = std::function<void(bool)>;
    using ConfigReadCallback = std::function<PersistentConfig(void)>;
    using ConfigWriteCallback = std::function<void(PersistentConfig)>;
    using ConfigCommandHandler = std::function<std::string(const std::string&)>;

    DltLogServer(StaticConfig static_config,
                 ConfigReadCallback reader,
                 ConfigWriteCallback writer,
                 bool enabled,
                 std::unique_ptr<ILogSender> log_sender = nullptr,
                 std::unique_ptr<IDiagnosticJobParser> parser = nullptr)
        : score::platform::datarouter::DltNonverboseHandlerType::IOutput(),
          DltVerboseHandler::IOutput(),
          FileTransferStreamHandlerType::IOutput(),
          config_mutex_{},
          filtering_enabled_{},
          dlt_output_enabled_{enabled},
          default_threshold_{},
          message_thresholds_{},
          channel_assignments_{},
          throughput_overall_{0},
          throughput_apps_{},
          static_config_{std::move(static_config)},
          channels_{},
          default_channel_{},
          coredump_channel_{std::nullopt},
          channel_nums_{},
          nvhandler_{*this},
          vhandler_{*this},
          fthandler_{*this},
          reader_callback_{reader},
          writer_callback_{writer},
          log_sender_(log_sender ? std::move(log_sender) : std::make_unique<LogSender>()),
          parser_(parser ? std::move(parser) : std::make_unique<DiagnosticJobParser>())
    {
        InitLogChannels();
        SysedrFactoryType sysedr_factory;
        sysedr_handler_ = sysedr_factory.CreateSysedrHandler();
    }

    virtual ~DltLogServer() = default;
    // Not possible to mock LogParser currently.
    // LCOV_EXCL_START
    std::vector<ILogParser::AnyHandler*> GetGlobalHandlers()
    {
        return {sysedr_handler_.get(), &nvhandler_};
    }

    std::vector<ILogParser::TypeHandlerBinding> GetTypeHandlerBindings()
    {
        return {{kPersistentRequestTypeName, sysedr_handler_.get()},
                {kLogEntryTypeName, &vhandler_},
                {kFileTransferTypeName, &fthandler_}};
    }

    // LCOV_EXCL_STOP

    void SetEnabledCallback(EnabledCallback enabled_callback = EnabledCallback())
    {
        enabled_callback_ = enabled_callback;
    }

    void SetDltOutputEnabled(bool enabled)
    {
        dlt_output_enabled_.store(enabled, std::memory_order_release);
    }

    void Flush()
    {
        for (auto& channel : channels_)
        {
            channel.Flush();
        }
    }

    double GetQuota(std::string name)
    {
        auto quota = throughput_apps_.find(DltidT(name));
        return quota == throughput_apps_.end() ? 1.0 : quota->second;
    }

    bool GetQuotaEnforcementEnabled() const
    {
        return static_config_.quota_enforcement_enabled;
    }

    SessionPtr NewConfigSession(score::platform::datarouter::ConfigSessionHandleType handle)
    {
        return score::platform::datarouter::DynamicConfigurationHandlerFactoryType().CreateConfigSession(
            std::move(handle), MakeConfigCommandHandler());
    }

    // Provide a delegate to handle config commands without exposing private methods
    ConfigCommandHandler MakeConfigCommandHandler()
    {
        return [this](const std::string& command) {
            return OnConfigCommand(command);
        };
    }

    // LCOV_EXCL_START : not possible to test log info output
    template <typename Logger>
    void ShowChannelStatistics(uint16_t series_num, Logger& stats_logger)
    {
        stats_logger.LogInfo() << "log stat for the channels #" << series_num;
        for (auto& dltlogchannel : channels_)
        {
            dltlogchannel.ShowStats(stats_logger);
        }
    }
    // LCOV_EXCL_STOP

    bool GetDltEnabled() const noexcept;

    std::string ReadLogChannelNames() override;
    std::string ResetToDefault() override;
    std::string StoreDltConfig() override;
    std::string SetTraceState() override;
    std::string SetDefaultTraceState() override;
    std::string SetLogChannelThreshold(DltidT channel, LoglevelT threshold) override;
    std::string SetLogLevel(DltidT app_id, DltidT ctx_id, ThresholdT threshold) override;
    std::string SetMessagingFilteringState(bool enabled) override;
    std::string SetDefaultLogLevel(LoglevelT level) override;
    std::string SetLogChannelAssignment(DltidT app_id,
                                        DltidT ctx_id,
                                        DltidT channel,
                                        AssignmentAction assignment_flag) override;
    std::string SetDltOutputEnable(bool enable) override;

    // This is used for test purpose only in google tests, to have an access to the private members.
    // Do not use this calls in implementation except unit tests.
    class DltLogServerTest;

  private:
    using KeyT = std::pair<DltidT, DltidT>;

    struct KeyHash
    {
      public:
        std::size_t operator()(const KeyT& k) const
        {
            auto low = static_cast<uint64_t>(k.first.GetHash());
            auto high = static_cast<uint64_t>(k.second.GetHash()) << 32;
            return std::hash<uint64_t>{}(high | low);
        }
    };

    using ChannelmaskT = std::bitset<32>;

    template <typename F>
    void FilterAndCall(DltidT app_id, DltidT ctx_id, mw::log::LogLevel log_level, F f)
    {
        ChannelmaskT assigned;
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            if (!IsAcceptedByFiltering(app_id, ctx_id, log_level))
            {
                return;
            }
            assigned = AssignedToChannels(app_id, ctx_id);
        }
        if (assigned.none())
        {
            f(channels_[default_channel_]);
        }
        else
        {
            for (size_t i = 0; i < channels_.size(); ++i)
            {
                if (assigned[i])
                {
                    f(channels_[i]);
                }
            }
        }
    }

    template <typename KeyMap>
    static std::optional<typename KeyMap::mapped_type> FindInKeyMap(const KeyMap& m, DltidT app_id, DltidT ctx_id)
    {
        auto p = m.find({app_id, ctx_id});
        if (p == m.end())
        {
            p = m.find({DltidT{}, ctx_id});
            if (p == m.end())
            {
                p = m.find({app_id, DltidT{}});
            }
        }
        if (p == m.end())
        {
            return std::nullopt;
        }
        else
        {
            return p->second;
        }
    }

    // should be called under the mutex
    bool IsAcceptedByFiltering(const DltidT app_id, const DltidT ctx_id, const mw::log::LogLevel log_level)
    {
        if (!filtering_enabled_)
        {
            return true;
        }
        const auto threshold = FindInKeyMap(message_thresholds_, app_id, ctx_id).value_or(default_threshold_);
        return log_level <= threshold;
    }

    // should be called under the mutex
    ChannelmaskT AssignedToChannels(DltidT app_id, DltidT ctx_id)
    {
        return FindInKeyMap(channel_assignments_, app_id, ctx_id).value_or(ChannelmaskT{});
    }

    std::mutex config_mutex_;

    bool filtering_enabled_;
    std::atomic<bool> dlt_output_enabled_;

    LoglevelT default_threshold_;
    std::unordered_map<KeyT, LoglevelT, KeyHash> message_thresholds_;
    std::unordered_map<KeyT, ChannelmaskT, KeyHash> channel_assignments_;

    double throughput_overall_;
    std::unordered_map<DltidT, double> throughput_apps_;

    StaticConfig static_config_;

    std::vector<DltLogChannel> channels_;
    size_t default_channel_;
    std::optional<uint8_t> coredump_channel_;
    std::unordered_map<DltidT, size_t> channel_nums_;

    score::platform::datarouter::DltNonverboseHandlerType nvhandler_;
    DltVerboseHandler vhandler_;
    FileTransferStreamHandlerType fthandler_;
    EnabledCallback enabled_callback_;
    ConfigReadCallback reader_callback_;
    ConfigWriteCallback writer_callback_;
    std::unique_ptr<ILogSender> log_sender_;
    std::unique_ptr<IDiagnosticJobParser> parser_;

    std::unique_ptr<ISysedrHandler> sysedr_handler_;

    bool IsOutputEnabled() const noexcept override final;

    void SendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                        uint32_t tmsp,
                        const void* data,
                        size_t size) override final;
    void SendVerbose(
        uint32_t tmsp,
        const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry) override final;
    void SendFtVerbose(score::cpp::span<const std::uint8_t> data,
                       mw::log::LogLevel loglevel,
                       DltidT app_id,
                       DltidT ctx_id,
                       uint8_t nor,
                       uint32_t tmsp) override final;

    void InitLogChannels(const bool reloading = false);
    void InitLogChannelsDefault(const bool reloading = false);

    void SaveDatabase();
    void ClearDatabase();

    void SetOutputEnabled(const bool enabled);

    std::string OnConfigCommand(const std::string& command);
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DAEMON_DLT_LOG_SERVER_H
