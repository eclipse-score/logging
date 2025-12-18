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
/*
    - this is private functions in this file so it cannot be test.
    - contains some concreate classes so we can not inject them.

*/
// LCOV_EXCL_START
void SocketServer::doWork(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime)
{
    SetThreadName();

    score::mw::log::Logger& stats_logger = score::mw::log::CreateLogger("STAT", "statistics");

    std::unique_ptr<IPersistentDictionary> pd = PersistentDictionaryFactoryType::Create(no_adaptive_runtime);
    const auto loadDlt = [&pd]() {
        return readDlt(*pd);
    };
    const auto storeDlt = [&pd](const score::logging::dltserver::PersistentConfig& config) {
        writeDlt(config, *pd);
    };
    bool isDltEnabled = readDltEnabled(*pd);
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
    isDltEnabled = true;
// coverity[autosar_cpp14_a16_0_1_violation] see above
#endif

    score::mw::log::LogInfo() << "Loaded output enable = " << isDltEnabled;
    const auto static_config = readStaticDlt(LOG_CHANNELS_PATH);
    if (!static_config.has_value())
    {
        score::mw::log::LogError() << static_config.error();
        score::mw::log::LogError() << "Error during parsing file " << LOG_CHANNELS_PATH
                                 << ", static config is not available, interrupt work";
        return;
    }
    /*
        Deviation from Rule A5-1-4:
        - A lambda expression object shall not outlive any of its reference captured objects.
        Justification:
        - dltServer and lambda are in the same scope.
    */
    // coverity[autosar_cpp14_a5_1_4_violation]
    score::logging::dltserver::DltLogServer dltServer(static_config.value(), loadDlt, storeDlt, isDltEnabled);
    const auto sourceSetup = [&dltServer](LogParser&& parser) {
        parser.set_filter_factory(getFilterFactory());
        dltServer.add_handlers(parser);
    };
    /*
        Deviation from Rule A5-1-4:
        - A lambda expression object shall not outlive any of its reference captured objects.
        Justification:
        - router and lambda are in the same scope.
    */
    // coverity[autosar_cpp14_a5_1_4_violation]
    DataRouter router(stats_logger, sourceSetup);
    const auto enableHandler = [&router, &pd, &dltServer](bool enable) {
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
        writeDltEnabled(enable, *pd);
        router.for_each_source_parser(
            [&dltServer, enable](LogParser& parser) {
                dltServer.update_handlers(parser, enable);
            },
            [&dltServer, enable] {
                dltServer.update_handlers_final(enable);
            },
            enable);
    };
    dltServer.set_enabled_callback(enableHandler);

    const auto factory = [&dltServer](const std::string& /*name*/, UnixDomainServer::SessionHandle handle) {
        return dltServer.new_config_session(score::platform::datarouter::ConfigSessionHandleType{std::move(handle)});
    };
    const UnixDomainSockAddr addr(score::logging::config::socket_address, true);
    /*
    Deviation from Rule A5-1-4:
    - A lambda expression object shall not outlive any of its reference captured objects.
    Justification:
    - server does not exist inside any lambda.
    */
    // coverity[autosar_cpp14_a5_1_4_violation: FALSE]
    UnixDomainServer server(addr, factory);

    // Try to create NvConfig from file using factory, fallback to empty config if it fails
    const auto nvConfig = [&stats_logger]() {
        auto nvConfigResult = score::mw::log::NvConfigFactory::CreateAndInit();
        if (nvConfigResult.has_value())
        {
            stats_logger.LogInfo() << "NvConfig loaded successfully";
            return std::move(nvConfigResult.value());
        }
        else
        {
            stats_logger.LogWarn() << "Failed to load NvConfig: " << nvConfigResult.error().Message();
            return score::mw::log::NvConfigFactory::CreateEmpty();
        }
    }();  // ← immediately invoked

    const auto mp_factory = [&router, &dltServer, &nvConfig](
                                const pid_t client_pid,
                                const score::mw::log::detail::ConnectMessageFromClient& conn,
                                score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle)
        -> std::unique_ptr<MessagePassingServer::ISession> {
        const auto appid_sv = conn.GetAppId().GetStringView();
        const std::string appid{appid_sv.data(), appid_sv.size()};
        const std::string shared_memory_file_name = ResolveSharedMemoryFileName(conn, appid);
        // The reason for banning is, because it's error-prone to use. One should use abstractions e.g. provided by
        // the C++ standard library. But these abstraction do not support exclusive access, which is why we created
        // this abstraction library.
        // NOLINTBEGIN(score-banned-function): See above.
        auto maybe_fd =
            score::os::Fcntl::instance().open(shared_memory_file_name.c_str(), score::os::Fcntl::Open::kReadOnly);
        // NOLINTEND(score-banned-function) it is among safety headers.
        if (!maybe_fd.has_value())
        {
            std::cerr << "message_session_factory: open(O_RDONLY) " << shared_memory_file_name << maybe_fd.error()
                      << std::endl;
            return std::unique_ptr<MessagePassingServer::ISession>();
        }

        const auto fd = maybe_fd.value();
        const auto quota = dltServer.get_quota(appid);
        const auto quotaEnforcementEnabled = dltServer.getQuotaEnforcementEnabled();
        const bool is_dlt_enabled = dltServer.GetDltEnabled();
        auto source_session = router.new_source_session(
            fd, appid, is_dlt_enabled, std::move(handle), quota, quotaEnforcementEnabled, client_pid, nvConfig);
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
    };

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

    uint16_t count = 0U;
    constexpr std::uint32_t statistics_freq_divider = statistics_log_period_us / throttle_time_us;
    constexpr std::uint32_t dlt_freq_divider = dlt_flush_period_us / throttle_time_us;

    while (!exit_requested.load())
    {
        usleep(throttle_time_us);

        if ((count % statistics_freq_divider) == 0U)
        {
            router.show_source_statistics(static_cast<uint16_t>(count / statistics_freq_divider));
            dltServer.show_channel_statistics(static_cast<uint16_t>(count / statistics_freq_divider), stats_logger);
        }
        if ((count % dlt_freq_divider) == 0U)
        {
            dltServer.flush();
        }
        ++count;
    }
}
// LCOV_EXCL_STOP

}  // namespace datarouter
}  // namespace platform
}  // namespace score
