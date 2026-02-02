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

#include "score/mw/log/detail/logging_identifier.h"

#include <score/utility.hpp>

#include <array>
#include <cstring>
#include <type_traits>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

enum class DatarouterMessageIdentifier : std::uint8_t
{
    kConnect = 0x00,
    kAcquireRequest = 0x01,
    kAcquireResponse = 0x02,
};

/// \brief Returns a pointer to the raw memory of a trivially copyable object as uint8_t*.
/// \tparam T The type of the object. Must be trivially copyable.
/// \param obj The object to get the memory pointer for.
/// \returns A pointer to the object's memory as uint8_t*.
template <typename T>
constexpr const std::uint8_t* AsBytes(const T& obj) noexcept
{
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
    return static_cast<const std::uint8_t*>(static_cast<const void*>(std::addressof(obj)));
}

/// \brief Serializes a trivially copyable message with a message identifier prefix.
/// \tparam MessageType The type of the message payload. Must be trivially copyable.
/// \param identifier The message identifier to prepend to the serialized data.
/// \param payload The message payload to serialize.
/// \returns A std::array containing the message identifier followed by the serialized payload.
template <typename MessageType>
constexpr std::array<std::uint8_t, sizeof(MessageType) + 1U> SerializeMessage(
    const DatarouterMessageIdentifier identifier,
    const MessageType& payload) noexcept
{
    static_assert(std::is_trivially_copyable<MessageType>::value, "MessageType must be trivially copyable");

    std::array<std::uint8_t, sizeof(MessageType) + 1U> message{};
    message[0] = score::cpp::to_underlying(identifier);
    auto destination_iterator = message.begin();
    std::advance(destination_iterator, 1U);
    std::ignore = std::copy_n(AsBytes(payload), sizeof(MessageType), destination_iterator);
    return message;
}

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
