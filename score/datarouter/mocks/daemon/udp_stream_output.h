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

#ifndef SCORE_DATAROUTER_MOCKS_DAEMON_UDP_STREAM_OUTPUT_H
#define SCORE_DATAROUTER_MOCKS_DAEMON_UDP_STREAM_OUTPUT_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "daemon/dltserver_common.h"
#include "score/os/socket.h"

#include <score/span.hpp>
#include <gmock/gmock.h>

namespace score
{
namespace logging
{
namespace dltserver
{

inline namespace mock
{
class UdpStreamOutput
{
  public:
    class Tester
    {
      public:
        static Tester*& Instance()
        {
            static Tester instance;
            static Tester* p_instance = &instance;  // overwritable
            return p_instance;
        }

        MOCK_METHOD(void, construct, (UdpStreamOutput*, const char*, uint16_t, const char*));
        MOCK_METHOD(void, MoveConstruct, (UdpStreamOutput*, UdpStreamOutput*));
        MOCK_METHOD(void, Destruct, (UdpStreamOutput*));

        MOCK_METHOD(score::cpp::expected_blank<score::os::Error>, Bind, (UdpStreamOutput*, const char*, uint16_t));
        MOCK_METHOD((score::cpp::expected<std::int64_t, score::os::Error>), Send, (UdpStreamOutput*, const iovec*, size_t));
        MOCK_METHOD((score::cpp::expected<std::int32_t, score::os::Error>), Send, (UdpStreamOutput*, score::cpp::span<mmsghdr>));
    };

    UdpStreamOutput(const char* dst_addr, uint16_t dst_port, const char* multicast_interface)
    {
        Tester::Instance()->construct(this, dst_addr, dst_port, multicast_interface);
    }
    ~UdpStreamOutput()
    {
        Tester::Instance()->Destruct(this);
    }
    UdpStreamOutput(const UdpStreamOutput&) = delete;
    UdpStreamOutput(UdpStreamOutput&& from) noexcept
    {
        Tester::Instance()->MoveConstruct(this, &from);
    }
    score::cpp::expected_blank<score::os::Error> Bind(const char* src_addr = nullptr, uint16_t src_port = 0)
    {
        return Tester::Instance()->Bind(this, src_addr, src_port);
    }
    score::cpp::expected<std::int64_t, score::os::Error> Send(const iovec* data, size_t size)
    {
        return Tester::Instance()->Send(this, data, size);
    }
    score::cpp::expected<std::int32_t, score::os::Error> Send(score::cpp::span<mmsghdr> mmsg_span)
    {
        return Tester::Instance()->Send(this, mmsg_span);
    }
};

}  // namespace mock

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_MOCKS_DAEMON_UDP_STREAM_OUTPUT_H
