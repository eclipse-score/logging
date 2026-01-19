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

#ifndef DLT_LOG_SERVER_H_
#define DLT_LOG_SERVER_H_

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

const std::string LOG_ENTRY_TYPE_NAME{"score::mw::log::detail::LogEntry"};
const std::string PERSISTENT_REQUEST_TYPE_NAME{"score::logging::internal::PersistentLoggingRequestStructure"};
const std::string FILE_TRANSFER_TYPE_NAME{"score::logging::FileTransferEntry"};
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

    DltLogServer(StaticConfig staticConfig,
                 ConfigReadCallback reader,
                 ConfigWriteCallback writer,
                 bool enabled,
                 std::unique_ptr<ILogSender> log_sender = nullptr,
                 std::unique_ptr<IDiagnosticJobParser> parser = nullptr)
        : score::platform::datarouter::DltNonverboseHandlerType::IOutput(),
          DltVerboseHandler::IOutput(),
          FileTransferStreamHandlerType::IOutput(),
          configMutex_{},
          filteringEnabled_{},
          dltOutputEnabled_{enabled},
          defaultThreshold_{},
          messageThresholds_{},
          channelAssignments_{},
          throughput_overall_{0},
          throughput_apps_{},
          staticConfig_{std::move(staticConfig)},
          channels_{},
          defaultChannel_{},
          coredumpChannel_{std::nullopt},
          channelNums_{},
          nvhandler_{*this},
          vhandler_{*this},
          fthandler{*this},
          readerCallback_{reader},
          writerCallback_{writer},
          log_sender_(log_sender ? std::move(log_sender) : std::make_unique<LogSender>()),
          parser_(parser ? std::move(parser) : std::make_unique<DiagnosticJobParser>())
    {
        init_log_channels();
        SysedrFactoryType sysedr_factory;
        sysedr_handler_ = sysedr_factory.CreateSysedrHandler();
    }

    virtual ~DltLogServer() = default;
    // Not possible to mock LogParser currently.
    // LCOV_EXCL_START
    void add_handlers(ILogParser& parser)
    {
        parser.add_global_handler(*sysedr_handler_);
        parser.add_type_handler(PERSISTENT_REQUEST_TYPE_NAME, *sysedr_handler_);

        if (dltOutputEnabled_)
        {
            // XXX only add handler for those which are accepted
            parser.add_global_handler(nvhandler_);
            parser.add_type_handler(LOG_ENTRY_TYPE_NAME, vhandler_);
            parser.add_type_handler(FILE_TRANSFER_TYPE_NAME, fthandler);
        }
    }

    void update_handlers(ILogParser& parser, bool enabled)
    {
        // protected by external mutex
        if (enabled)
        {
            parser.add_global_handler(nvhandler_);
            parser.add_type_handler(LOG_ENTRY_TYPE_NAME, vhandler_);
            parser.add_type_handler(FILE_TRANSFER_TYPE_NAME, fthandler);
        }
        else
        {
            parser.remove_global_handler(nvhandler_);
            parser.remove_type_handler(LOG_ENTRY_TYPE_NAME, vhandler_);
            parser.remove_type_handler(FILE_TRANSFER_TYPE_NAME, fthandler);
        }
    }
    // LCOV_EXCL_STOP

    void set_enabled_callback(EnabledCallback enabledCallback = EnabledCallback())
    {
        enabledCallback_ = enabledCallback;
    }

    void update_handlers_final(bool enabled)
    {
        // protected by external mutex
        dltOutputEnabled_ = enabled;
    }

    void flush()
    {
        for (auto& channel : channels_)
        {
            channel.flush();
        }
    }

    double get_quota(std::string name)
    {
        auto quota = throughput_apps_.find(dltid_t(name));
        return quota == throughput_apps_.end() ? 1.0 : quota->second;
    }

    bool getQuotaEnforcementEnabled() const
    {
        return staticConfig_.quotaEnforcementEnabled;
    }

    SessionPtr new_config_session(score::platform::datarouter::ConfigSessionHandleType handle)
    {
        return score::platform::datarouter::DynamicConfigurationHandlerFactoryType().CreateConfigSession(
            std::move(handle), make_config_command_handler());
    }

    // Provide a delegate to handle config commands without exposing private methods
    ConfigCommandHandler make_config_command_handler()
    {
        return [this](const std::string& command) {
            return on_config_command(command);
        };
    }

    // LCOV_EXCL_START : not possible to test log info output
    template <typename Logger>
    void show_channel_statistics(uint16_t series_num, Logger& stats_logger_)
    {
        stats_logger_.LogInfo() << "log stat for the channels #" << series_num;
        for (auto& dltlogchannel : channels_)
        {
            dltlogchannel.show_stats(stats_logger_);
        }
    }
    // LCOV_EXCL_STOP

    bool GetDltEnabled() const noexcept;

    const std::string ReadLogChannelNames() override;
    const std::string ResetToDefault() override;
    const std::string StoreDltConfig() override;
    const std::string SetTraceState() override;
    const std::string SetDefaultTraceState() override;
    const std::string SetLogChannelThreshold(dltid_t channel, loglevel_t threshold) override;
    const std::string SetLogLevel(dltid_t appId, dltid_t ctxId, threshold_t threshold) override;
    const std::string SetMessagingFilteringState(bool enabled) override;
    const std::string SetDefaultLogLevel(loglevel_t level) override;
    const std::string SetLogChannelAssignment(dltid_t appId,
                                              dltid_t ctxId,
                                              dltid_t channel,
                                              AssignmentAction assignment_flag) override;
    const std::string SetDltOutputEnable(bool enable) override;

    // This is used for test purpose only in google tests, to have an access to the private members.
    // Do not use this calls in implementation except unit tests.
    class DltLogServerTest;

  private:
    using key_t = std::pair<dltid_t, dltid_t>;

    struct key_hash
    {
      public:
        std::size_t operator()(const key_t& k) const
        {
            auto low = static_cast<uint64_t>(k.first.value);
            auto high = static_cast<uint64_t>(k.second.value) << 32;
            return std::hash<uint64_t>{}(high | low);
        }
    };

    using channelmask_t = std::bitset<32>;

    template <typename F>
    void filterAndCall(dltid_t appId, dltid_t ctxId, mw::log::LogLevel logLevel, F f)
    {
        channelmask_t assigned;
        {
            std::lock_guard<std::mutex> lock(configMutex_);
            if (!isAcceptedByFiltering(appId, ctxId, logLevel))
            {
                return;
            }
            assigned = assignedToChannels(appId, ctxId);
        }
        if (assigned.none())
        {
            f(channels_[defaultChannel_]);
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
    static std::optional<typename KeyMap::mapped_type> findInKeyMap(const KeyMap& m, dltid_t appId, dltid_t ctxId)
    {
        auto p = m.find({appId, ctxId});
        if (p == m.end())
        {
            p = m.find({dltid_t{}, ctxId});
            if (p == m.end())
            {
                p = m.find({appId, dltid_t{}});
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
    bool isAcceptedByFiltering(const dltid_t appId, const dltid_t ctxId, const mw::log::LogLevel logLevel)
    {
        if (!filteringEnabled_)
        {
            return true;
        }
        const auto threshold = findInKeyMap(messageThresholds_, appId, ctxId).value_or(defaultThreshold_);
        return logLevel <= threshold;
    }

    // should be called under the mutex
    channelmask_t assignedToChannels(dltid_t appId, dltid_t ctxId)
    {
        return findInKeyMap(channelAssignments_, appId, ctxId).value_or(channelmask_t{});
    }

    std::mutex configMutex_;

    bool filteringEnabled_;
    bool dltOutputEnabled_;

    loglevel_t defaultThreshold_;
    std::unordered_map<key_t, loglevel_t, key_hash> messageThresholds_;
    std::unordered_map<key_t, channelmask_t, key_hash> channelAssignments_;

    double throughput_overall_;
    std::unordered_map<dltid_t, double> throughput_apps_;

    StaticConfig staticConfig_;

    std::vector<DltLogChannel> channels_;
    size_t defaultChannel_;
    std::optional<uint8_t> coredumpChannel_;
    std::unordered_map<dltid_t, size_t> channelNums_;

    score::platform::datarouter::DltNonverboseHandlerType nvhandler_;
    DltVerboseHandler vhandler_;
    FileTransferStreamHandlerType fthandler;
    EnabledCallback enabledCallback_;
    ConfigReadCallback readerCallback_;
    ConfigWriteCallback writerCallback_;
    std::unique_ptr<ILogSender> log_sender_;
    std::unique_ptr<IDiagnosticJobParser> parser_;

    std::unique_ptr<ISysedrHandler> sysedr_handler_;

    void sendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                        uint32_t tmsp,
                        const void* data,
                        size_t size) override final;
    void sendVerbose(
        uint32_t tmsp,
        const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry) override final;
    void sendFTVerbose(score::cpp::span<const std::uint8_t> data,
                       mw::log::LogLevel loglevel,
                       dltid_t appId,
                       dltid_t ctxId,
                       uint8_t nor,
                       uint32_t tmsp) override final;

    void init_log_channels(const bool reloading = false);
    void init_log_channels_default(const bool reloading = false);

    void save_database();
    void clear_database();

    void set_output_enabled(const bool enabled);

    const std::string on_config_command(const std::string& command);
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // DLT_LOG_SERVER_H_
