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

#ifndef LOGGING_SOCKETSERVER_H
#define LOGGING_SOCKETSERVER_H

#include "daemon/dlt_log_server.h"
#include "daemon/message_passing_server.h"
#include "logparser/logparser.h"
#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/logging.h"
#include "score/datarouter/datarouter/data_router.h"
#include "score/datarouter/src/persistency/i_persistent_dictionary.h"
#include "unix_domain/unix_domain_server.h"

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

// Forward declaration for testing
namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
class ConnectMessageFromClient;
}
}  // namespace log
}  // namespace mw
}  // namespace score

namespace score
{
namespace os
{
class Pthread;
}  // namespace os
}  // namespace score

#include "score/message_passing/client_factory.h"
#include "score/message_passing/server_factory.h"

namespace score
{
namespace platform
{
namespace datarouter
{

using ServerFactory = score::message_passing::ServerFactory;
using ClientFactory = score::message_passing::ClientFactory;

class SocketServer
{
  public:
    struct PersistentStorageHandlers
    {
        std::function<score::logging::dltserver::PersistentConfig()> load_dlt;
        std::function<void(const score::logging::dltserver::PersistentConfig&)> store_dlt;
        bool is_dlt_enabled;
    };

    static void run(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime)
    {
        static SocketServer server;
        server.doWork(exit_requested, no_adaptive_runtime);
    }
    //  static void run(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime);

    /// @internal Test helpers - do not use in production code

    static PersistentStorageHandlers InitializePersistentStorage(
        std::unique_ptr<IPersistentDictionary>& persistent_dictionary);

    static std::unique_ptr<score::logging::dltserver::DltLogServer> CreateDltServer(
        const PersistentStorageHandlers& storage_handlers);

    static DataRouter::SourceSetupCallback CreateSourceSetupHandler(score::logging::dltserver::DltLogServer& dlt_server);

    // Static helper functions for testing lambda bodies
    static void UpdateParserHandlers(score::logging::dltserver::DltLogServer& dlt_server,
                                     score::platform::internal::ILogParser& parser,
                                     bool enable);

    static void UpdateHandlersFinal(score::logging::dltserver::DltLogServer& dlt_server, bool enable);

    static std::unique_ptr<score::platform::internal::UnixDomainServer::ISession> CreateConfigSession(
        score::logging::dltserver::DltLogServer& dlt_server,
        score::platform::internal::UnixDomainServer::SessionHandle handle);

    static std::function<void(bool)> CreateEnableHandler(DataRouter& router,
                                                         IPersistentDictionary& persistent_dictionary,
                                                         score::logging::dltserver::DltLogServer& dlt_server);

    static std::unique_ptr<score::platform::internal::UnixDomainServer> CreateUnixDomainServer(
        score::logging::dltserver::DltLogServer& dlt_server);

    static std::unique_ptr<score::platform::internal::MessagePassingServer::ISession> CreateMessagePassingSession(
        DataRouter& router,
        score::logging::dltserver::DltLogServer& dlt_server,
        const score::mw::log::NvConfig& nv_config,
        const pid_t client_pid,
        const score::mw::log::detail::ConnectMessageFromClient& conn,
        score::cpp::pmr::unique_ptr<score::platform::internal::daemon::ISessionHandle> handle);

    static score::mw::log::NvConfig LoadNvConfig(
        score::mw::log::Logger& stats_logger,
        const std::string& config_path = "/bmw/platform/opt/datarouter/etc/class-id.json");

    static void RunEventLoop(const std::atomic_bool& exit_requested,
                             DataRouter& router,
                             score::logging::dltserver::DltLogServer& dlt_server,
                             score::mw::log::Logger& stats_logger);
    // Testable helper functions
    static void SetThreadName() noexcept;
    static void SetThreadName(score::os::Pthread& pthread_instance) noexcept;
    static std::string ResolveSharedMemoryFileName(const score::mw::log::detail::ConnectMessageFromClient& conn,
                                                   const std::string& appid);

  private:
    void doWork(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime);
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // LOGGING_SOCKETSERVER_H
