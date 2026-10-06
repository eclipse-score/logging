/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#include "score/mw/log/detail/syslog/syslog_recorder_factory.h"

#include <score/assert.hpp>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
std::unique_ptr<Recorder> SyslogRecorderFactory::CreateConcreteLogRecorder(
    const Configuration& config,
    score::cpp::pmr::memory_resource* memory_resource)
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(memory_resource != nullptr);
    auto backend = CreateSystemBackend(config, memory_resource);  // LCOV_EXCL_LINE : no branches to test
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(backend != nullptr);
    constexpr bool kCheckLogLevelForConsole = false;
    return std::make_unique<TextRecorder>(config, std::move(backend), kCheckLogLevelForConsole);
}

std::unique_ptr<Backend> SyslogRecorderFactory::CreateSystemBackend(const Configuration& config,
                                                                    score::cpp::pmr::memory_resource* memory_resource)
{
    auto syslog_instance = score::os::Syslog::Default(memory_resource);
    SCORE_LANGUAGE_FUTURECPP_ASSERT_PRD(syslog_instance != nullptr);
    return std::make_unique<SyslogBackend>(config.GetNumberOfSlots(),
                                           LogRecord{config.GetSlotSizeInBytes()},
                                           config.GetAppId(),
                                           std::move(syslog_instance));
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
