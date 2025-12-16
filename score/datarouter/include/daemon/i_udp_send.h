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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_UDP_SEND_H
#define SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_UDP_SEND_H

#include "score/os/errno.h"
#include <score/utility.hpp>

namespace score
{
namespace logging
{
namespace dltserver
{

class IUdpSend
{
  public:
    virtual ~IUdpSend() = default;

    virtual score::cpp::expected<std::int32_t, score::os::Error> send(score::cpp::span<const uint8_t> /*mmsg*/) noexcept = 0;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score
#endif  // SCORE_PAS_LOGGING_INCLUDE_DAEMON_I_UDP_SEND_H
