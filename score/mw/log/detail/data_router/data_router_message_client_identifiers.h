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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_IDENTIFIERS_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_IDENTIFIERS_H

#include "score/mw/log/detail/logging_identifier.h"
#include <iostream>
#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class MsgClientIdentifiers
{
  public:
    MsgClientIdentifiers(const std::string& receiver_id,
                         const pid_t this_process_id,
                         const LoggingIdentifier& app_id,
                         const uid_t datarouter_uid,
                         const uid_t uid);

    const std::string& GetReceiverID() const noexcept;
    const pid_t& GetThisProcID() const noexcept;
    const LoggingIdentifier& GetAppID() const noexcept;
    uid_t GetDatarouterUID() const noexcept;
    uid_t GetUID() const noexcept;

  private:
    std::string receiver_id_;
    pid_t this_process_id_;
    LoggingIdentifier app_id_;
    uid_t datarouter_uid_;
    uid_t uid_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_IDENTIFIERS_H
