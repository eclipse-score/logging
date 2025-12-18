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

#ifndef UDP_STREAM_OUTPUT_H_
#define UDP_STREAM_OUTPUT_H_

#include "score/os/pthread.h"
#include "score/os/socket_impl.h"
#include "score/datarouter/network/vlan.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <pthread.h>
#include <errno.h>
#include <memory>

namespace score
{
namespace logging
{
namespace dltserver
{
// Suppress "AUTOSAR C++14 M3-2-3". The rule states: "A type, object or function that is used in
// multiple translation units shall be declared in one and only one file.".
// Rationale: This is the actual class but also there is a mock class with the name in mocks folder.
// coverity[autosar_cpp14_m3_2_3_violation]
class UdpStreamOutput
{
  public:
    UdpStreamOutput(const char* dstAddr,
                    uint16_t dstPort,
                    const char* multicastInterface,
                    std::unique_ptr<score::os::Socket> socket_instance = std::make_unique<score::os::SocketImpl>(),
                    score::os::Vlan& vlan = score::os::Vlan::instance());

    ~UdpStreamOutput();
    UdpStreamOutput(const UdpStreamOutput&) = delete;
    UdpStreamOutput(UdpStreamOutput&& from) noexcept;

    score::cpp::expected_blank<score::os::Error> bind(const char* srcAddr = nullptr, uint16_t srcPort = 0) noexcept;

    score::cpp::expected<std::int32_t, score::os::Error> send(score::cpp::span<mmsghdr> mmsg) noexcept;

    // Used to send single big message:
    score::cpp::expected<std::int64_t, score::os::Error> send(const iovec* iovec_tab, const size_t size) noexcept;

  private:
    int socket_;
    struct sockaddr_in dst_;
    std::unique_ptr<score::os::Pthread> pthread_;
    std::unique_ptr<score::os::Socket> socket_instance_;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // UDP_STREAM_OUTPUT_H_
