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

#include "score/mw/log/detail/data_router/data_router_message_client_factory_impl.h"

#include "score/assert.hpp"
#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/detail/data_router/data_router_message_client_impl.h"
#include "score/mw/log/legacy_non_verbose_api/tracing.h"
#include <iostream>
#include <sstream>
namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

namespace
{
std::string GetClientIdentifier(const std::string& logging_client_identifier)
{
    return "/" + logging_client_identifier;
}

}  // namespace

DatarouterMessageClientFactoryImpl::DatarouterMessageClientFactoryImpl(
    const Configuration& config,
    std::unique_ptr<MessagePassingFactory> message_passing_factory,
    MsgClientUtils msg_client_utils) noexcept
    : DatarouterMessageClientFactory{},
      created_once_{false},
      config_{config},
      message_passing_factory_{std::move(message_passing_factory)},
      msg_client_utils_{std::move(msg_client_utils)}
{
}

std::unique_ptr<DatarouterMessageClient> DatarouterMessageClientFactoryImpl::CreateOnce(
    const std::string& identifer,
    const std::string& mwsr_file_name)
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(created_once_ == false, "The factory shall be used single time only.");
    created_once_ = true;

    const pid_t this_process_pid = msg_client_utils_.GetUnistd().getpid();

    const uid_t uid = msg_client_utils_.GetUnistd().getuid();

    return std::make_unique<DatarouterMessageClientImpl>(
        MsgClientIdentifiers(
            GetClientIdentifier(identifer),
            this_process_pid,
            LoggingIdentifier{config_.GetAppId()},
            /*
             The max number of digits of DataRouter id is 4 digits,
             so we need 2 bytes to represent it , but uid_t is 4 byte int, so no data loss when casting.
            */
            // coverity[autosar_cpp14_a4_7_1_violation]
            static_cast<uid_t>(config_.GetDataRouterUid()),
            uid),
        MsgClientBackend(score::platform::logger::instance().GetSharedMemoryWriter(),
                         mwsr_file_name,
                         std::move(message_passing_factory_),
                         config_.GetDynamicDatarouterIdentifiers()),
        std::move(msg_client_utils_));
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
