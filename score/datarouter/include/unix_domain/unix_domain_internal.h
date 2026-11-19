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

#ifndef SCORE_DATAROUTER_INCLUDE_UNIX_DOMAIN_UNIX_DOMAIN_INTERNAL_H
#define SCORE_DATAROUTER_INCLUDE_UNIX_DOMAIN_UNIX_DOMAIN_INTERNAL_H

#include <memory>
#include <string>

#include <score/optional.hpp>
#include <score/span.hpp>

#include "score/os/utils/signal.h"
#include "unix_domain/unix_domain_common.h"

namespace score
{
namespace platform
{
namespace internal
{

// Public API names used unqualified by the implementation in this namespace.
using unix_domain::AncillaryDataFileHandleReceptionCallback;
using unix_domain::RecvSocketMessage;
using unix_domain::SendSocketMessage;
using unix_domain::SharedMemoryFileHandle;
using unix_domain::UnixDomainSockAddr;

enum class MessageType : std::uint8_t
{
    kDefault = 0,
    kSharedMemoryFileHandle = 1,
};

struct SocketMessangerHeader  //  __attribute__((packed))  //  TODO: deal with packed
{
    uint16_t len;
    MessageType type;
    std::int32_t pid;
};

/* @data - data to be send over the socket */
void SendAncillaryDataOverSocket(int connection_file_descriptor, score::cpp::span<std::uint8_t> data);

std::optional<std::string> RecvSocketMessage(
    std::int32_t socket_fd,
    score::cpp::optional<SharedMemoryFileHandle>& file_handle,
    score::cpp::optional<std::int32_t>& peer_pid,
    AncillaryDataFileHandleReceptionCallback ancillary_data_process = AncillaryDataFileHandleReceptionCallback{});

void SetupSignals(const std::unique_ptr<score::os::Signal>& signal);

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_UNIX_DOMAIN_UNIX_DOMAIN_INTERNAL_H
