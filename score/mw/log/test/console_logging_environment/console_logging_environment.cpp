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
#include "score/mw/log/test/console_logging_environment/console_logging_environment.h"

#include "score/mw/log/detail/registry_aware_recorder_factory.h"

#include "score/mw/log/runtime.h"

namespace score
{
namespace mw
{
namespace log
{

void ConsoleLoggingEnvironment::SetUp()
{
    score::mw::log::detail::Configuration config{};
    config.SetLogMode({score::mw::LogMode::kConsole});
    config.SetDefaultConsoleLogLevel(score::mw::log::LogLevel::kVerbose);
    recorder_ = score::mw::log::detail::RegistryAwareRecorderFactory().CreateRecorderFromLogMode(
        score::mw::LogMode::kConsole, config);

    score::mw::log::detail::Runtime::SetRecorder(recorder_.get());
}

void ConsoleLoggingEnvironment::TearDown()
{
    score::mw::log::detail::Runtime::SetRecorder(nullptr);
    recorder_.reset();
}

}  // namespace log
}  // namespace mw
}  // namespace score
