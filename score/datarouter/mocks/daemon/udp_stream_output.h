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
        static Tester*& instance()
        {
            static Tester instance;
            static Tester* p_instance = &instance;  // overwritable
            return p_instance;
        }

        MOCK_METHOD(void, construct, (UdpStreamOutput*, const char*, uint16_t, const char*));
        MOCK_METHOD(void, move_construct, (UdpStreamOutput*, UdpStreamOutput*));
        MOCK_METHOD(void, destruct, (UdpStreamOutput*));

        MOCK_METHOD(score::cpp::expected_blank<score::os::Error>, bind, (UdpStreamOutput*, const char*, uint16_t));
        MOCK_METHOD((score::cpp::expected<std::int64_t, score::os::Error>), send, (UdpStreamOutput*, const iovec*, size_t));
        MOCK_METHOD((score::cpp::expected<std::int32_t, score::os::Error>), send, (UdpStreamOutput*, score::cpp::span<mmsghdr>));
    };

    UdpStreamOutput(const char* dstAddr, uint16_t dstPort, const char* multicastInterface)
    {
        Tester::instance()->construct(this, dstAddr, dstPort, multicastInterface);
    }
    ~UdpStreamOutput()
    {
        Tester::instance()->destruct(this);
    }
    UdpStreamOutput(const UdpStreamOutput&) = delete;
    UdpStreamOutput(UdpStreamOutput&& from) noexcept
    {
        Tester::instance()->move_construct(this, &from);
    }
    score::cpp::expected_blank<score::os::Error> bind(const char* srcAddr = nullptr, uint16_t srcPort = 0)
    {
        return Tester::instance()->bind(this, srcAddr, srcPort);
    }
    score::cpp::expected<std::int64_t, score::os::Error> send(const iovec* data, size_t size)
    {
        return Tester::instance()->send(this, data, size);
    }
    score::cpp::expected<std::int32_t, score::os::Error> send(score::cpp::span<mmsghdr> mmsgSpan)
    {
        return Tester::instance()->send(this, mmsgSpan);
    }
};

}  // namespace mock

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // UDP_STREAM_OUTPUT_H_
