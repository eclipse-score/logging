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

#include "score/mw/log/detail/data_router/data_router_messages.h"

#include "score/utility.hpp"
#include "data_router_messages.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

bool operator==(const ConnectMessageFromClient& lhs, const ConnectMessageFromClient& rhs) noexcept
{
    return ((lhs.appid_ == rhs.appid_) && (lhs.uid_ == rhs.uid_)) &&
           ((lhs.use_dynamic_identifier_ == rhs.use_dynamic_identifier_) && (lhs.random_part_ == rhs.random_part_));
}

bool operator!=(const ConnectMessageFromClient& lhs, const ConnectMessageFromClient& rhs) noexcept
{
    return !(lhs == rhs);
}

ConnectMessageFromClient::ConnectMessageFromClient(const LoggingIdentifier& appid,
                                                   uid_t uid,
                                                   bool use_dynamic_identifier,
                                                   const std::array<std::string::value_type, 6>& random_part) noexcept
    : appid_(appid), uid_(uid), use_dynamic_identifier_(use_dynamic_identifier), random_part_(random_part)
{
}

void ConnectMessageFromClient::SetAppId(const LoggingIdentifier& appid) noexcept
{
    appid_ = appid;
}

void ConnectMessageFromClient::SetUid(uid_t uid) noexcept
{
    uid_ = uid;
}

void ConnectMessageFromClient::SetUseDynamicIdentifier(bool use_dynamic_identifier) noexcept
{
    use_dynamic_identifier_ = use_dynamic_identifier;
}

void ConnectMessageFromClient::SetRandomPart(const std::array<std::string::value_type, 6>& random_part) noexcept
{
    random_part_ = random_part;
}

uid_t ConnectMessageFromClient::GetUid() const noexcept
{
    return uid_;
}

bool ConnectMessageFromClient::GetUseDynamicIdentifier() const noexcept
{
    return use_dynamic_identifier_;
}

std::array<std::string::value_type, 6> ConnectMessageFromClient::GetRandomPart() const noexcept
{
    return random_part_;
}

LoggingIdentifier ConnectMessageFromClient::GetAppId() const noexcept
{
    return appid_;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
