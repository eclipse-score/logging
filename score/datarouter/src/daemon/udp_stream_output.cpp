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

#include "score/datarouter/include/daemon/udp_stream_output.h"

#include "daemon/dltserver_common.h"
#include "score/os/unistd.h"
#include "score/datarouter/network/vlan.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <system_error>

score::logging::dltserver::UdpStreamOutput::UdpStreamOutput(const char* dstAddr,
                                                          uint16_t dstPort,
                                                          const char* multicastInterface,
                                                          std::unique_ptr<score::os::Socket> socket_instance,
                                                          score::os::Vlan& vlan)
    : socket_{-1}, dst_{}, pthread_{score::os::Pthread::Default()}, socket_instance_{std::move(socket_instance)}
{
    dst_.sin_family = AF_INET;
    dst_.sin_port = htons(dstPort);
    if (dstAddr == nullptr || inet_aton(dstAddr, &dst_.sin_addr) == 0)
    {
        dst_.sin_addr.s_addr = INADDR_ANY;
    }

    {
        const auto ret = socket_instance_->socket(score::os::Socket::Domain::kIPv4, SOCK_DGRAM, IPPROTO_UDP);
        if (ret.has_value())
        {
            socket_ = ret.value();
        }
    }
    {
        u_char loop = 1;
        const auto ret_setsockopt_multicastloop =
            socket_instance_->setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if (!ret_setsockopt_multicastloop.has_value())
        {
            const auto error_string = ret_setsockopt_multicastloop.error().ToString();
            std::cerr << "ERROR: (UDP) socket cannot reuse address: " << error_string << std::endl;
        }
    }

    {
        constexpr int32_t SOCK_OPT_ENABLE = 1;
        const auto ret_setsockopt_reuseport =
            socket_instance_->setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &SOCK_OPT_ENABLE, sizeof(int));
        if (!ret_setsockopt_reuseport.has_value())
        {
            const auto error_string = ret_setsockopt_reuseport.error().ToString();
            std::cerr << "ERROR: (UDP) socket cannot reuse port: " << error_string << std::endl;
        }
    }

    {
        //  On QNX when the buffer is smaller than the message we are trying to send _sendto_ failes with negative
        //  code 64 K is the maximum length of the DLT message.
        constexpr std::int32_t socket_sndbuf_size = 64L * 1024UL;
        const auto ret_setsockopt_sndbuf = socket_instance_->setsockopt(
            socket_, SOL_SOCKET, SO_SNDBUF, &socket_sndbuf_size, sizeof(socket_sndbuf_size));
        if (!ret_setsockopt_sndbuf.has_value())
        {
            const auto error_string = ret_setsockopt_sndbuf.error().ToString();
            std::cerr << "ERROR: (UDP) socket cannot set buffer size: " << error_string << std::endl;
        }
    }

    {
        constexpr int32_t SOCK_OPT_ENABLE_REUSEADDR = 1;
        const auto ret_setsockopt_reuseaddress =
            socket_instance_->setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &SOCK_OPT_ENABLE_REUSEADDR, sizeof(int));
        if (!ret_setsockopt_reuseaddress.has_value())
        {
            const auto error_string = ret_setsockopt_reuseaddress.error().ToString();
            std::cerr << "ERROR: (UDP) socket cannot reuse address: " << error_string << std::endl;
        }
    }

    if (multicastInterface != nullptr && std::strlen(multicastInterface) != 0)
    {
        struct in_addr addr{};
        if (inet_aton(multicastInterface, &addr) != 0)
        {
            if (setsockopt(socket_, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)) == -1)
            {
                std::cerr << "ERROR: (UDP) socket cannot use multicast interface: "
                          /*
                              Deviation from Rule M19-3-1:
                              - The error indicator errno shall not be used.
                              Justification:
                              - Using library-defined macro to ensure correct operation.
                          */
                          // coverity[autosar_cpp14_m19_3_1_violation]
                          << std::system_category().message(errno) << std::endl;
            }
        }
        else
        {
            std::cerr << "ERROR: Invalid multicast interface address: " << multicastInterface
                      << " "
                      // coverity[autosar_cpp14_m19_3_1_violation]
                      << std::system_category().message(errno) << std::endl;
        }
    }

    constexpr std::uint8_t kDltPcpPriority = 1u;
    const auto pcp_result = vlan.SetVlanPriorityOfSocket(kDltPcpPriority, socket_);
    if (!pcp_result.has_value())
    {
        const auto error = pcp_result.error().ToString();
        std::cerr << "ERROR: Setting PCP Priority: " << error << std::endl;
    }
}

score::logging::dltserver::UdpStreamOutput::~UdpStreamOutput()
{
    if (socket_ != -1)
    {
        // The reason for banning is, because it's error-prone to use. One should use abstractions e.g. provided by
        // the C++ standard library. But these abstraction do not support exclusive access, which is why we created
        // this abstraction library.
        // NOLINTNEXTLINE(score-banned-function): See above.
        std::ignore = score::os::Unistd::instance().close(socket_);
    }
}

score::logging::dltserver::UdpStreamOutput::UdpStreamOutput(UdpStreamOutput&& from) noexcept
    : socket_{from.socket_},
      dst_{from.dst_},
      pthread_{std::move(from.pthread_)},
      socket_instance_{std::move(from.socket_instance_)}
{
    from.socket_ = -1;
}

score::cpp::expected_blank<score::os::Error> score::logging::dltserver::UdpStreamOutput::bind(const char* srcAddr,
                                                                                   uint16_t srcPort) noexcept
{
    struct sockaddr_in src{};
    src.sin_family = AF_INET;
    src.sin_port = htons(srcPort);
    if (srcAddr == nullptr || inet_aton(srcAddr, &src.sin_addr) == 0)
    {
        src.sin_addr.s_addr = INADDR_ANY;
    }

    // Step 1: Convert sockaddr_in* to void*
    // - This is safe because void* is a generic pointer type and does not change the actual memory layout.
    void* sockaddr_void_ptr = static_cast<void*>(&src);

    // Step 2: Convert void* to sockaddr*
    // - This is safe because sockaddr_in and sockaddr have the same initial structure (`__SOCKADDR_COMMON`).
    // - POSIX guarantees this cast is valid for socket functions
    /*
        Deviation from Rule M5-2-8:
        - Rule M5-2-8 (required, implementation, automated)
        An object with integer type or pointer to void type shall not be converted
        to an object with pointer type.
        Justification:
        - This is safe because sockaddr_in and sockaddr have the same initial structure (`__SOCKADDR_COMMON`).
    */
    // coverity[autosar_cpp14_m5_2_8_violation]
    const auto addr = static_cast<const struct sockaddr*>(sockaddr_void_ptr);

    const auto ret = socket_instance_->bind(socket_, addr, sizeof(src));
    if (!ret.has_value())
    {
        auto errstr = ret.error().ToString();
        std::cerr << "ERROR: (UDP) socket cannot bind to (" << (srcAddr != nullptr ? srcAddr : "any") << ":" << srcPort
                  << "): " << errstr << std::endl;
    }
    return ret;
}

score::cpp::expected<std::int32_t, score::os::Error> score::logging::dltserver::UdpStreamOutput::send(
    score::cpp::span<mmsghdr> mmsg) noexcept
{
    for (auto& msg : mmsg)
    {
        msg.msg_hdr.msg_name = static_cast<void*>(&dst_);
        msg.msg_hdr.msg_namelen = sizeof(dst_);
        msg.msg_hdr.msg_control = nullptr;
        msg.msg_hdr.msg_controllen = 0UL;
    }
    const auto ret = socket_instance_->sendmmsg(
        socket_, mmsg.data(), static_cast<uint32_t>(mmsg.size()), score::os::Socket::MessageFlag::kNone);
    return ret;
}

// Used to send single big message:
score::cpp::expected<std::int64_t, score::os::Error> score::logging::dltserver::UdpStreamOutput::send(const iovec* iovec_tab,
                                                                                           const size_t size) noexcept
{
    struct msghdr msg{};
    msg.msg_name = static_cast<void*>(&dst_);
    msg.msg_namelen = sizeof(dst_);

    /*
        Deviation from Rule A5-2-3:
        - A cast shall not remove any const or volatile
            qualification from the type of a pointer or reference.
        Justification:
        - const_cast is necessary to remove const qualifier in order to adjust constant iovec_tab to msg_iov.
    */
    // coverity[autosar_cpp14_a5_2_3_violation]
    msg.msg_iov = const_cast<iovec*>(iovec_tab);

    // condition can not be reached since we both variable are the same type.
    // LCOV_EXCL_START
    // Ensure that 'size' fits within the range of 'msg_iovlen'
    if (size > static_cast<std::size_t>(std::numeric_limits<decltype(msghdr::msg_iovlen)>::max()))
    {
        return score::cpp::make_unexpected(score::os::Error::createFromErrno(EOVERFLOW));
    }
    // LCOV_EXCL_STOP

    msg.msg_iovlen = static_cast<decltype(msghdr::msg_iovlen)>(size);

    msg.msg_control = nullptr;
    msg.msg_controllen = 0UL;

    const auto ret = socket_instance_->sendmsg(socket_, &msg, score::os::Socket::MessageFlag::kNone);
    return ret;
}
