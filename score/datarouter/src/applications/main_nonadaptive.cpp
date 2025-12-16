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

// We exclude this main function from coverage because it is only used
// for demonstration or testing purposes and not part of production code.
// LCOV_EXCL_START
#include <atomic>

#include "score/os/utils/signal_impl.h"
#include "score/mw/log/logging.h"

#include <score/span.hpp>
#include <datarouter_app.h>
#include <options.h>

namespace
{
std::atomic_bool exit_requested;

void signal_handler(std::int32_t /* signal */)
{
    exit_requested = true;
}
}  // namespace

int main(std::int32_t argc, const char* argv[])
{
    score::cpp::span<const char*> args(argv, static_cast<score::cpp::span<const char*>::size_type>(argc));
    if (!score::logging::options::Options::parse(argc, const_cast<char* const*>(argv)))
    {
        //  Error messages have already been logged, just say goodbye.
        score::mw::log::LogError() << args.front() << "Terminating because of errors in command line";

        return 1;
    }

    score::os::SignalImpl sig{};
    // score::os wrappers are better suited for dependency injection,
    // So suppressed here as it is safely used, and it is among safety headers.
    // NOLINTNEXTLINE(score-banned-function) see comment above.
    sig.signal(SIGTERM, signal_handler);
    score::logging::datarouter::datarouter_app_init();
    score::logging::datarouter::datarouter_app_run(exit_requested);
    score::logging::datarouter::datarouter_app_shutdown();

    return 0;
}
// LCOV_EXCL_STOP
