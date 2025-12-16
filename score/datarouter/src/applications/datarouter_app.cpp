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

#include "options.h"

#include "daemon/socketserver.h"

#include "score/mw/log/logging.h"

#include <iostream>
#include <string>

namespace
{
const std::string program_version{"Version 0.1s"};
}  // namespace

namespace score
{
namespace logging
{
namespace datarouter
{

using namespace score::platform::datarouter;

void datarouter_app_init()
{
    score::mw::log::LogInfo() << "datarouter application" << program_version << "starting";
}

void datarouter_app_run(const std::atomic_bool& exit_requested)
{
    const score::logging::options::Options& opts = score::logging::options::Options::get();

    if (opts.do_nothing())
    {
        return;
    }

    if (opts.print_version())
    {
        std::cout << program_version << "\n";
        return;
    }

    if (opts.no_adaptive_runtime())
    {
        score::mw::log::LogInfo()
            << "datarouter will not use the Vector stack. Persistency features will not be available.";
    }

    score::mw::log::LogInfo() << "datarouter successfully completed initialization and goes live!";

    SocketServer::run(exit_requested, opts.no_adaptive_runtime());
}

void datarouter_app_shutdown()
{
    score::mw::log::LogInfo() << "shutting down";
}

}  // namespace datarouter
}  // namespace logging
}  // namespace score
