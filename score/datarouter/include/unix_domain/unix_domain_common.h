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

#ifndef UNIX_DOMAIN_COMMON_H_
#define UNIX_DOMAIN_COMMON_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <memory>
#include <string>

#include <score/callback.hpp>
#include <score/optional.hpp>
#include <score/span.hpp>
#include <score/string_view.hpp>

#include "score/os/utils/signal.h"

namespace score
{
namespace platform
{
namespace internal
{
using SharedMemoryFileHandle = std::int32_t;

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

class UnixDomainSockAddr
{
  public:
    UnixDomainSockAddr(const std::string& path, bool isAbstract);
    const char* get_address_string()
    {
        std::uint8_t offset = is_abstract() ? 1U : 0U;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index) -bounds already checked via is_abstract()
        return &(addr_.sun_path[offset]);
    }

    bool is_abstract()
    {
        return !addr_.sun_path[0];
    }

    struct sockaddr_un addr_;
};

using AncillaryDataFileHandleReceptionCallback =
    score::cpp::callback<score::cpp::optional<SharedMemoryFileHandle>(const score::cpp::span<std::uint8_t> buffer)>;

/* @file_handle - is optional and used to pass file descriptor on Linux */
void send_socket_message(std::int32_t connection_file_descriptor,
                         score::cpp::string_view message,
                         score::cpp::optional<SharedMemoryFileHandle> file_handle = score::cpp::nullopt);

/* @data - data to be send over the socket */
void SendAncillaryDataOverSocket(int connection_file_descriptor, score::cpp::span<std::uint8_t> data);

/* Returns: Message string. Possibly zero-sized when pinging or timeout occurs. score::cpp::nullopt is returned when error
 * occurs.
 * @ancillary_data_process - callback may be used to convert ancillary data to file descriptor but it is not the only
 * way to retrive fd. For Linux part of the implementation it may be passed by native socket ancillary data. Overload
 * function with reduced number of arguments is provided when file handle is not expected.
 */
score::cpp::optional<std::string> recv_socket_message(
    std::int32_t socket_fd,
    AncillaryDataFileHandleReceptionCallback ancillary_data_process = AncillaryDataFileHandleReceptionCallback{});

score::cpp::optional<std::string> recv_socket_message(
    std::int32_t socket_fd,
    score::cpp::optional<SharedMemoryFileHandle>& file_handle,
    score::cpp::optional<std::int32_t>& peer_pid,
    AncillaryDataFileHandleReceptionCallback ancillary_data_process = AncillaryDataFileHandleReceptionCallback{});

void SetupSignals(const std::unique_ptr<score::os::Signal>& signal);

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // UNIX_DOMAIN_COMMON_H_
