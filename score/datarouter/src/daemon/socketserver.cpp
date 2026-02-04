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
#include "daemon/socketserver_filter_factory.h"
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

using score::platform::internal::LogParser;
using score::platform::internal::MessagePassingServer;
using score::platform::internal::UnixDomainSockAddr;

namespace
{
constexpr auto* LOG_CHANNELS_PATH = "./etc/log-channels.json";

constexpr std::uint32_t statistics_log_period_us{10000000U};
constexpr std::uint32_t dlt_flush_period_us{100000U};
constexpr std::uint32_t throttle_time_us{100000U};
/*
    - this is local functions in this file so it cannot be tested
*/
// LCOV_EXCL_START
void SetThreadName() noexcept
{
    auto ret = score::os::Pthread::instance().setname_np(score::os::Pthread::instance().self(), "socketserver");
    if (!ret.has_value())
    {
        auto errstr = ret.error().ToString();
        std::cerr << "pthread_setname_np: " << errstr << std::endl;
    }
}

std::string ResolveSharedMemoryFileName(const score::mw::log::detail::ConnectMessageFromClient& conn,
                                        const std::string& appid)
{
    std::string return_file_name_string;

    // constuct the file from the 6 random chars
    if (true == conn.GetUseDynamicIdentifier())
    {
        std::string random_part;
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
// LCOV_EXCL_STOP

}  // namespace

SocketServer::PersistentStorageHandlers SocketServer::InitializePersistentStorage(
    std::unique_ptr<IPersistentDictionary>& persistent_dictionary)
{
    PersistentStorageHandlers handlers;

    handlers.load_dlt = [&persistent_dictionary]() {
        return readDlt(*persistent_dictionary);
    };

    handlers.store_dlt = [&persistent_dictionary](const score::logging::dltserver::PersistentConfig& config) {
        writeDlt(config, *persistent_dictionary);
    };

    handlers.is_dlt_enabled = readDltEnabled(*persistent_dictionary);

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
    const auto static_config = readStaticDlt(LOG_CHANNELS_PATH);
    if (!static_config.has_value())
    {
        score::mw::log::LogError() << static_config.error();
        score::mw::log::LogError() << "Error during parsing file " << std::string_view{LOG_CHANNELS_PATH}
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

DataRouter::SourceSetupCallback SocketServer::CreateSourceSetupHandler(
    score::logging::dltserver::DltLogServer& dlt_server)
{
    /*
        Deviation from Rule A5-1-4:
        - A lambda expression object shall not outlive any of its reference captured objects.
        Justification:
        - dltServer and lambda are in the same scope.
    */
    // coverity[autosar_cpp14_a5_1_4_violation]
    return [&dlt_server](score::platform::internal::ILogParser&& parser) {
        parser.set_filter_factory(getFilterFactory());
        dlt_server.add_handlers(parser);
    };
}

// Static helper: Update handlers for each parser
void SocketServer::UpdateParserHandlers(score::logging::dltserver::DltLogServer& dlt_server,
                                        score::platform::internal::ILogParser& parser,
                                        bool enable)
{
    dlt_server.update_handlers(parser, enable);
}

// Static helper: Final update after all parsers processed
void SocketServer::UpdateHandlersFinal(score::logging::dltserver::DltLogServer& dlt_server, bool enable)
{
    dlt_server.update_handlers_final(enable);
}

// Static helper: Create a new config session from Unix domain handle
std::unique_ptr<UnixDomainServer::ISession> SocketServer::CreateConfigSession(
    score::logging::dltserver::DltLogServer& dlt_server,
    UnixDomainServer::SessionHandle handle)
{
    return dlt_server.new_config_session(score::platform::datarouter::ConfigSessionHandleType{std::move(handle)});
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
        writeDltEnabled(enable, persistent_dictionary);
        router.for_each_source_parser(
            std::bind(&SocketServer::UpdateParserHandlers, std::ref(dlt_server), std::placeholders::_1, enable),
            std::bind(&SocketServer::UpdateHandlersFinal, std::ref(dlt_server), enable),
            enable);
    };
}

std::unique_ptr<score::platform::internal::UnixDomainServer> SocketServer::CreateUnixDomainServer(
    score::logging::dltserver::DltLogServer& dlt_server)
{
    const auto factory = std::bind(&SocketServer::CreateConfigSession, std::ref(dlt_server), std::placeholders::_2);

    const UnixDomainSockAddr addr(score::logging::config::socket_address, true);
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
        auto nvConfigResult = score::mw::log::NvConfigFactory::CreateAndInit(config_path);
        if (nvConfigResult.has_value())
        {
            stats_logger.LogInfo() << "NvConfig loaded successfully";
            return std::move(nvConfigResult.value());
        }
        else
        {
            stats_logger.LogWarn() << "Failed to load NvConfig: " << nvConfigResult.error().Message();
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
    const auto quota = dlt_server.get_quota(appid);
    const auto quotaEnforcementEnabled = dlt_server.getQuotaEnforcementEnabled();
    const bool is_dlt_enabled = dlt_server.GetDltEnabled();
    auto source_session = router.new_source_session(
        fd, appid, is_dlt_enabled, std::move(handle), quota, quotaEnforcementEnabled, client_pid, nv_config);
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
    constexpr std::uint32_t statistics_freq_divider = statistics_log_period_us / throttle_time_us;
    constexpr std::uint32_t dlt_freq_divider = dlt_flush_period_us / throttle_time_us;

    while (!exit_requested.load())
    {
        usleep(throttle_time_us);

        if ((count % statistics_freq_divider) == 0U)
        {
            router.show_source_statistics(static_cast<uint16_t>(count / statistics_freq_divider));
            dlt_server.show_channel_statistics(static_cast<uint16_t>(count / statistics_freq_divider), stats_logger);
        }
        if ((count % dlt_freq_divider) == 0U)
        {
            dlt_server.flush();
        }
        ++count;
    }
}

void SocketServer::doWork(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime)
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

    // Create data router with source setup handler
    const auto source_setup = CreateSourceSetupHandler(*dlt_server);
    /*
        Deviation from Rule A5-1-4:
        - A lambda expression object shall not outlive any of its reference captured objects.
        Justification:
        - router and lambda are in the same scope.
    */
    // coverity[autosar_cpp14_a5_1_4_violation]
    DataRouter router(stats_logger, source_setup);

    // Create and set enable handler
    const auto enable_handler = CreateEnableHandler(router, *pd, *dlt_server);
    dlt_server->set_enabled_callback(enable_handler);

    // Create Unix domain server for config sessions
    auto unix_domain_server = CreateUnixDomainServer(*dlt_server);

    // Load NvConfig
    const score::mw::log::NvConfig nv_config = LoadNvConfig(stats_logger);

    // Create message passing factory using std::bind directly
    const auto mp_factory = std::bind(&SocketServer::CreateMessagePassingSession,
                                      std::ref(router),
                                      std::ref(*dlt_server),
                                      std::ref(nv_config),
                                      std::placeholders::_1,   // client_pid
                                      std::placeholders::_2,   // conn
                                      std::placeholders::_3);  // handle

    // Create message passing server with thread pool
    // As documented in aas/mw/com/message_passing/design/README.md, the Receiver implementation will use just 1 thread
    // from the thread pool for MQueue (Linux). For Resource Manager (QNX), it is supposed to use 2 threads. If it
    // cannot allocate the second thread, it will work with just one thread, with reduced functionality (still enough
    // for our use case, where every client's Sender runs on a dedicated thread) and likely with higher latency.
    score::concurrency::ThreadPool executor{2};
    /*
    Deviation from Rule A5-1-4:
    - A lambda expression object shall not outlive any of its reference captured objects.
    Justification:
    - mp_server does not exist inside any lambda.
    */
    // coverity[autosar_cpp14_a5_1_4_violation: FALSE]
    MessagePassingServer mp_server(mp_factory, executor);

    // Run main event loop
    RunEventLoop(exit_requested, router, *dlt_server, stats_logger);
}
// LCOV_EXCL_STOP

}  // namespace datarouter
}  // namespace platform
}  // namespace score
