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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGES_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGES_H

#include "score/mw/com/message_passing/message.h"

#include "score/mw/log/detail/logging_identifier.h"

#include <array>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

enum class DatarouterMessageIdentifier : score::mw::com::message_passing::MessageId
{
    kConnect = 0x00,
    kAcquireRequest = 0x01,
    kAcquireResponse = 0x02,
};

score::mw::com::message_passing::MessageId ToMessageId(const DatarouterMessageIdentifier& message_id) noexcept;

class ConnectMessageFromClient
{
    LoggingIdentifier appid_{};
    uid_t uid_{};
    bool use_dynamic_identifier_{};
    std::array<std::string::value_type, 6> random_part_{};

  public:
    ConnectMessageFromClient() noexcept = default;
    ConnectMessageFromClient(const LoggingIdentifier& appid,
                             uid_t uid,
                             bool use_dynamic_identifier,
                             const std::array<std::string::value_type, 6>& random_part) noexcept;

    void SetAppId(const LoggingIdentifier& appid) noexcept;
    void SetUid(uid_t uid) noexcept;
    void SetUseDynamicIdentifier(bool use_dynamic_identifier) noexcept;
    void SetRandomPart(const std::array<std::string::value_type, 6>& random_part) noexcept;
    LoggingIdentifier GetAppId() const noexcept;
    uid_t GetUid() const noexcept;
    bool GetUseDynamicIdentifier() const noexcept;
    std::array<std::string::value_type, 6> GetRandomPart() const noexcept;

    ConnectMessageFromClient(const ConnectMessageFromClient&) = delete;
    ConnectMessageFromClient& operator=(const ConnectMessageFromClient&) noexcept = default;
    ConnectMessageFromClient(ConnectMessageFromClient&&) = delete;
    ConnectMessageFromClient& operator=(ConnectMessageFromClient&&) = delete;
    ~ConnectMessageFromClient() noexcept = default;

    friend bool operator==(const ConnectMessageFromClient&, const ConnectMessageFromClient&) noexcept;
    friend bool operator!=(const ConnectMessageFromClient&, const ConnectMessageFromClient&) noexcept;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGES_H
