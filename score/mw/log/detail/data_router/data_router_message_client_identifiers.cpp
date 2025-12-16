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

#include "score/mw/log/detail/data_router/data_router_message_client_identifiers.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

MsgClientIdentifiers::MsgClientIdentifiers(const std::string& receiver_id,
                                           const pid_t this_process_id,
                                           const LoggingIdentifier& app_id,
                                           const uid_t datarouter_uid,
                                           const uid_t uid)
    : receiver_id_{receiver_id},
      this_process_id_{this_process_id},
      app_id_{app_id},
      datarouter_uid_{datarouter_uid},
      uid_{uid}
{
}

const std::string& MsgClientIdentifiers::GetReceiverID() const noexcept
{
    return receiver_id_;
}

const pid_t& MsgClientIdentifiers::GetThisProcID() const noexcept
{
    return this_process_id_;
}

const LoggingIdentifier& MsgClientIdentifiers::GetAppID() const noexcept
{
    return app_id_;
}

uid_t MsgClientIdentifiers::GetDatarouterUID() const noexcept
{
    return datarouter_uid_;
}

uid_t MsgClientIdentifiers::GetUID() const noexcept
{
    return uid_;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
