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

#include "daemon/socketserver.h"

#include "daemon/dlt_log_server.h"
#include "daemon/message_passing_server.h"
#include "daemon/socketserver_config.h"
#include "logparser/logparser.h"

#include "score/datarouter/daemon_communication/session_handle_interface.h"
#include "score/datarouter/datarouter/data_router.h"
#include "score/datarouter/include/applications/datarouter_feature_config.h"
#include "unix_domain/unix_domain_common.h"
#include "unix_domain/unix_domain_server.h"

#include "score/concurrency/thread_pool.h"
#include "score/os/fcntl.h"
#include "score/os/pthread.h"
#include "score/os/unistd.h"
#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/configuration/nvconfigfactory.h"

// Constants
#include "data_router_cfg.h"

#include <score/math.hpp>
#include <functional>
#include <iostream>

namespace score
{
namespace platform
{
namespace datarouter
{

using score::platform::internal::MessagePassingServer;
using score::platform::internal::UnixDomainSockAddr;

namespace
{
constexpr auto* kLogChannelsPath = "./etc/log-channels.json";

constexpr std::uint32_t kStatisticsLogPeriodUs{10000000U};
constexpr std::uint32_t kDltFlushPeriodUs{100000U};
constexpr std::uint32_t kThrottleTimeUs{100000U};

}  // namespace

void SocketServer::SetThreadName() noexcept
{
    SetThreadName(score::os::Pthread::instance());
}

void SocketServer::SetThreadName(score::os::Pthread& pthread_instance) noexcept
{
    auto ret = pthread_instance.setname_np(pthread_instance.self(), "socketserver");
    if (!ret.has_value())
    {
        auto errstr = ret.error().ToString();
        std::cerr << "pthread_setname_np: " << errstr << std::endl;
    }
}

std::string SocketServer::ResolveSharedMemoryFileName(const score::mw::log::detail::ConnectMessageFromClient& conn,
                                                      const std::string& appid)
{
    std::string return_file_name_string;

    // constuct the file from the 6 random chars
    if (true == conn.GetUseDynamicIdentifier())
    {
        // The LCOV considered the below line as uncovered which impossible according to the code flow, For the quality
        // team argumentation, it may related to Ticket-213937
        std::string random_part{};  // LCOV_EXCL_LINE
        for (const auto& s : conn.GetRandomPart())
        {
            random_part += s;
        }
        return_file_name_string = std::string("/tmp/logging-") + random_part;
    }
    else
    {
        return_file_name_string = std::string("/tmp/logging.") + appid + "." + std::to_string(conn.GetUid());
    }
    return_file_name_string += ".shmem";
    return return_file_name_string;
}

SocketServer::PersistentStorageHandlers SocketServer::InitializePersistentStorage(
    std::unique_ptr<IPersistentDictionary>& persistent_dictionary)
{
    PersistentStorageHandlers handlers;

    auto* pd_ptr = persistent_dictionary.get();
    handlers.load_dlt = [pd_ptr]() {
        return ReadDlt(*pd_ptr);
    };
    handlers.store_dlt = [pd_ptr](const score::logging::dltserver::PersistentConfig& config) {
        WriteDlt(config, *pd_ptr);
    };
    handlers.is_dlt_enabled = ReadDltEnabled(*persistent_dictionary);

/*
    Deviation from Rule A16-0-1:
    - Rule A16-0-1 (required, implementation, automated)
    The pre-processor shall only be used for unconditional and conditional file
    inclusion and include guards, and using the following directives: (1) #ifndef,
    #ifdef, (3) #if, (4) #if defined, (5) #elif, (6) #else, (7) #define, (8) #endif, (9)
    #include.
    Justification:
    - Feature Flag to enable/disable DLT Logging in dev builds.
*/
// coverity[autosar_cpp14_a16_0_1_violation]
#ifdef DLT_OUTPUT_ENABLED
    // TODO: will be reworked in Ticket-207823
    handlers.is_dlt_enabled = true;
// coverity[autosar_cpp14_a16_0_1_violation] see above
#endif

    score::mw::log::LogInfo() << "Loaded output enable = " << handlers.is_dlt_enabled;

    return handlers;
}

std::unique_ptr<score::logging::dltserver::DltLogServer> SocketServer::CreateDltServer(
    const PersistentStorageHandlers& storage_handlers)
{
    const auto static_config = ReadStaticDlt(kLogChannelsPath);
    if (!static_config.has_value())
    {
        score::mw::log::LogError() << static_config.error();
        score::mw::log::LogError() << "Error during parsing file " << std::string_view{kLogChannelsPath}
                                 << ", static config is not available, interrupt work";
        return nullptr;
    }

    /*
        Deviation from Rule A5-1-4:
        - A lambda expression object shall not outlive any of its reference captured objects.
        Justification:
        - dltServer and lambda are in the same scope.
    */
    // coverity[autosar_cpp14_a5_1_4_violation]
    return std::make_unique<score::logging::dltserver::DltLogServer>(
        static_config.value(), storage_handlers.load_dlt, storage_handlers.store_dlt, storage_handlers.is_dlt_enabled);
}

std::unique_ptr<score::platform::internal::ILogParserFactory> SocketServer::CreateLogParserFactory(
    score::logging::dltserver::DltLogServer& dlt_server)
{
    class DltLogParserFactory : public score::platform::internal::ILogParserFactory
    {
      public:
        explicit DltLogParserFactory(score::logging::dltserver::DltLogServer& dlt_server) : dlt_server_(dlt_server) {}

        std::unique_ptr<score::platform::internal::ILogParser> Create(const score::mw::log::NvConfig& nv_config) override
        {
            auto global_handlers = dlt_server_.GetGlobalHandlers();
            score::platform::internal::LogParser::HandleRequestMap handle_request_map;
            for (auto& binding : dlt_server_.GetTypeHandlerBindings())
            {
                handle_request_map.emplace(std::move(binding.type_name), binding.handler);
            }
            return std::make_unique<score::platform::internal::LogParser>(
                nv_config, std::move(global_handlers), std::move(handle_request_map));
        }

      private:
        score::logging::dltserver::DltLogServer& dlt_server_;
    };

    return std::make_unique<DltLogParserFactory>(dlt_server);
}

// Static helper: Create a new config session from Unix domain handle
std::unique_ptr<UnixDomainServer::ISession> SocketServer::CreateConfigSession(
    score::logging::dltserver::DltLogServer& dlt_server,
    UnixDomainServer::SessionHandle handle)
{
    return dlt_server.NewConfigSession(score::platform::datarouter::ConfigSessionHandleType{std::move(handle)});
}

std::function<void(bool)> SocketServer::CreateEnableHandler(DataRouter& router,
                                                            IPersistentDictionary& persistent_dictionary,
                                                            score::logging::dltserver::DltLogServer& dlt_server)
{
    /*
        Deviation from Rule A5-1-4:
        - A lambda expression object shall not outlive any of its reference captured objects.
        Justification:
        - router and lambda are in the same scope.
    */
    // coverity[autosar_cpp14_a5_1_4_violation]
    return [&router, &persistent_dictionary, &dlt_server](bool enable) {
/*
    Deviation from Rule A16-0-1:
    - Rule A16-0-1 (required, implementation, automated)
    The pre-processor shall only be used for unconditional and conditional file
    inclusion and include guards, and using the following directives: (1) #ifndef,
    #ifdef, (3) #if, (4) #if defined, (5) #elif, (6) #else, (7) #define, (8) #endif, (9)
    #include.
    Justification:
    - Feature Flag to enable/disable DLT Logging in dev builds.
*/
// coverity[autosar_cpp14_a16_0_1_violation]
#ifdef DLT_OUTPUT_ENABLED
        // TODO: will be reworked in Ticket-207823
        enable = true;
// coverity[autosar_cpp14_a16_0_1_violation] see above
#endif
        std::cerr << "DRCMD enable callback called with " << enable << std::endl;
        score::mw::log::LogWarn() << "Changing output enable to " << enable;
        WriteDltEnabled(enable, persistent_dictionary);
        router.ForEachSource(enable);
        dlt_server.SetDltOutputEnabled(enable);
    };
}

std::unique_ptr<score::platform::internal::UnixDomainServer> SocketServer::CreateUnixDomainServer(
    score::logging::dltserver::DltLogServer& dlt_server)
{
    const auto factory = [&dlt_server](const std::string& /*name*/, UnixDomainServer::SessionHandle handle) {
        return SocketServer::CreateConfigSession(dlt_server, std::move(handle));
    };

    const UnixDomainSockAddr addr(score::logging::config::kSocketAddress, true);
    /*
    Deviation from Rule A5-1-4:
    - A lambda expression object shall not outlive any of its reference captured objects.
    Justification:
    - server does not exist inside any lambda.
    */
    // coverity[autosar_cpp14_a5_1_4_violation: FALSE]
    return std::make_unique<UnixDomainServer>(addr, factory);
}

score::mw::log::NvConfig SocketServer::LoadNvConfig(score::mw::log::Logger& stats_logger, const std::string& config_path)
{
    if constexpr (score::platform::datarouter::kNonVerboseDltEnabled)
    {
        auto nv_config_result = score::mw::log::NvConfigFactory::CreateAndInit(config_path);
        if (nv_config_result.has_value())
        {
            stats_logger.LogInfo() << "NvConfig loaded successfully";
            return std::move(nv_config_result.value());
        }
        else
        {
            stats_logger.LogWarn() << "Failed to load NvConfig: " << nv_config_result.error().Message();
        }
    }
    return score::mw::log::NvConfigFactory::CreateEmpty();
}

// Static helper: Create a message passing session from connection info
std::unique_ptr<MessagePassingServer::ISession> SocketServer::CreateMessagePassingSession(
    DataRouter& router,
    score::logging::dltserver::DltLogServer& dlt_server,
    const score::mw::log::NvConfig& nv_config,
    const pid_t client_pid,
    const score::mw::log::detail::ConnectMessageFromClient& conn,
    score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle)
{
    const auto appid_sv = conn.GetAppId().GetStringView();
    const std::string appid{appid_sv.data(), appid_sv.size()};
    const std::string shared_memory_file_name = ResolveSharedMemoryFileName(conn, appid);
    // The reason for banning is, because it's error-prone to use. One should use abstractions e.g. provided by
    // the C++ standard library. But these abstraction do not support exclusive access, which is why we created
    // this abstraction library.
    // NOLINTBEGIN(score-banned-function): See above.
    auto maybe_fd = score::os::Fcntl::instance().open(shared_memory_file_name.c_str(), score::os::Fcntl::Open::kReadOnly);
    // NOLINTEND(score-banned-function) it is among safety headers.
    if (!maybe_fd.has_value())
    {
        std::cerr << "message_session_factory: open(O_RDONLY) " << shared_memory_file_name << maybe_fd.error()
                  << std::endl;
        return std::unique_ptr<MessagePassingServer::ISession>();
    }

    const auto fd = maybe_fd.value();
    const auto quota = dlt_server.GetQuota(appid);
    const auto quota_enforcement_enabled = dlt_server.GetQuotaEnforcementEnabled();
    const bool is_dlt_enabled = dlt_server.GetDltEnabled();
    auto source_session = router.NewSourceSession(
        fd, appid, is_dlt_enabled, std::move(handle), quota, quota_enforcement_enabled, client_pid, nv_config);
    // The reason for banning is, because it's error-prone to use. One should use abstractions e.g. provided by
    // the C++ standard library. But these abstraction do not support exclusive access, which is why we created
    // this abstraction library.
    // NOLINTNEXTLINE(score-banned-function): See above.
    const auto close_result = score::os::Unistd::instance().close(fd);
    if (close_result.has_value() == false)
    {
        std::cerr << "message_session_factory: close(" << shared_memory_file_name
                  << ") failed: " << close_result.error() << std::endl;
    }
    return source_session;
}

/*
    RunEventLoop and doWork are integration-level orchestration functions.
    They are tested through integration tests. All individual functions they call
    (InitializePersistentStorage, CreateDltServer, CreateEnableHandler, etc.)
    are already tested at 100% coverage in unit tests.
*/
// LCOV_EXCL_START
void SocketServer::RunEventLoop(const std::atomic_bool& exit_requested,
                                DataRouter& router,
                                score::logging::dltserver::DltLogServer& dlt_server,
                                score::mw::log::Logger& stats_logger)
{
    uint16_t count = 0U;
    constexpr std::uint32_t kStatisticsFreqDivider = kStatisticsLogPeriodUs / kThrottleTimeUs;
    constexpr std::uint32_t kDltFreqDivider = kDltFlushPeriodUs / kThrottleTimeUs;

    while (!exit_requested.load())
    {
        usleep(kThrottleTimeUs);

        if ((count % kStatisticsFreqDivider) == 0U)
        {
            router.ShowSourceStatistics(static_cast<uint16_t>(count / kStatisticsFreqDivider));
            dlt_server.ShowChannelStatistics(static_cast<uint16_t>(count / kStatisticsFreqDivider), stats_logger);
        }
        if ((count % kDltFreqDivider) == 0U)
        {
            dlt_server.Flush();
        }
        ++count;
    }
}

void SocketServer::DoWork(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime)
{
    SetThreadName();

    score::mw::log::Logger& stats_logger = score::mw::log::CreateLogger("STAT", "statistics");

    // Initialize persistent storage
    std::unique_ptr<IPersistentDictionary> pd = PersistentDictionaryFactoryType::Create(no_adaptive_runtime);
    const PersistentStorageHandlers storage_handlers = InitializePersistentStorage(pd);

    // Create DLT server
    auto dlt_server = CreateDltServer(storage_handlers);
    if (!dlt_server)
    {
        return;
    }

    // Create data router with log parser factory
    auto log_parser_factory = CreateLogParserFactory(*dlt_server);
    DataRouter router(stats_logger, std::move(log_parser_factory));

    // Create and set enable handler
    const auto enable_handler = CreateEnableHandler(router, *pd, *dlt_server);
    dlt_server->SetEnabledCallback(enable_handler);

    // Create Unix domain server for config sessions
    auto unix_domain_server = CreateUnixDomainServer(*dlt_server);

    // Load NvConfig
    const score::mw::log::NvConfig nv_config = LoadNvConfig(stats_logger);

    // Create message passing factory
    const auto mp_factory = [&router, &dlt_server, &nv_config](
                                const pid_t client_pid,
                                const score::mw::log::detail::ConnectMessageFromClient& conn,
                                score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle) {
        return SocketServer::CreateMessagePassingSession(
            router, *dlt_server, nv_config, client_pid, conn, std::move(handle));
    };

    std::shared_ptr<ServerFactory> server_factory = std::make_shared<ServerFactory>();
    std::shared_ptr<ClientFactory> client_factory =
        std::make_shared<ClientFactory>(/*server_factory_->GetEngine() Ticket-234313*/);

    /*
    Deviation from Rule A5-1-4:
    - A lambda expression object shall not outlive any of its reference captured objects.
    Justification:
    - mp_server does not exist inside any lambda.
    */
    // coverity[autosar_cpp14_a5_1_4_violation: FALSE]
    MessagePassingServer mp_server(mp_factory, std::move(server_factory), std::move(client_factory));

    // Run main event loop
    RunEventLoop(exit_requested, router, *dlt_server, stats_logger);
}
// LCOV_EXCL_STOP

}  // namespace datarouter
}  // namespace platform
}  // namespace score
