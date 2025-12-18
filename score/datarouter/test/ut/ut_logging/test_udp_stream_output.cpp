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

#include "score/os/mocklib/socketmock.h"

#include "gtest/gtest.h"
#include <score/span.hpp>
#include <gmock/gmock.h>
#include <sstream>

namespace score
{
namespace logging
{
namespace dltserver
{
namespace
{

using namespace testing;

class UdpStreamOutputFixture : public testing::Test
{
  protected:
    void SetUp() override
    {
        sock_mock_ = std::make_unique<score::os::SocketMock>();
    }
    void TearDown() override {}

    const char* addr_ = "192.168.1.21";
    uint16_t port_ = 8000;
    const char* multicast_interface_ = "192.168.10.3";

    std::unique_ptr<score::os::SocketMock> sock_mock_;
    std::unique_ptr<UdpStreamOutput> stream_output_;
};

TEST(UdpStreamOutput, ConstructionAndDestructionOnStack)
{
    {
        // We don't care about the argument values in this test.
        UdpStreamOutput stream_output{"192.168.1.21", 9000, "192.168.1.21"};
    }
}

TEST(UdpStreamOutput, ConstructionAndDestructionOnStackwithdstAddrAsNullptr)
{
    {
        // We don't care about the argument values in this test.
        UdpStreamOutput stream_output{nullptr, 9000, "192.168.1.21"};
    }
}

TEST(UdpStreamOutput, InvalidIpForMultInterface)
{
    {
        // We don't care about the argument values in this test.
        UdpStreamOutput stream_output{"192.168.1.21", 9000, "192.1685.1.21"};
    }
}

TEST(UdpStreamOutput, MoveConstructor_ConstructionAndDestructionOnStack)
{
    {
        // We don't care about the argument values in this test.
        UdpStreamOutput stream_output{"192.168.1.21", 9000, "192.168.1.21"};
        UdpStreamOutput stream_output_moved = std::move(stream_output);
    }
}

TEST(UdpStreamOutput, ConstructionAndDestructionOnHeap)
{
    // We don't care about the argument values in this test.
    std::unique_ptr<UdpStreamOutput> stream_output{};
    ASSERT_NO_FATAL_FAILURE(stream_output = std::make_unique<UdpStreamOutput>("192.168.1.21", 9000, "192.168.1.21"));
    EXPECT_NE(stream_output, nullptr);
    stream_output.reset();
}

TEST_F(UdpStreamOutputFixture, SetsockoptMethodShallNotReturnValueInCaseOfFailure)
{
    // When mocking socket's bind to return error.
    EXPECT_CALL(*sock_mock_, setsockopt(_, _, _, _, _))
        .Times(Exactly(4))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));

    std::stringstream buffer;
    std::streambuf* oldCerr = std::cerr.rdbuf(buffer.rdbuf());
    // And instantiating a UdpStreamOutput instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    std::cerr.rdbuf(oldCerr);
    std::string output = buffer.str();
    EXPECT_THAT(output, ::testing::HasSubstr("ERROR: (UDP) socket cannot"));
}

TEST_F(UdpStreamOutputFixture, BindMethodShallNotReturnValueInCaseOfFailure)
{
    // When mocking socket's bind to return error.
    EXPECT_CALL(*sock_mock_, bind(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));

    // And instantiating a UdpStreamOutput instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    // And calling UdpStreamOutput's bind method.
    auto ret = stream_output_->bind();

    // It shall fail.
    EXPECT_FALSE(ret.has_value());
}

TEST_F(UdpStreamOutputFixture, BindMethodShallReturnValueIfItSucceeded)
{
    // When mocking socket's bind to return blank.
    EXPECT_CALL(*sock_mock_, bind(_, _, _)).Times(Exactly(1)).WillOnce([]() -> score::cpp::blank {
        return {};
    });

    // And instantiating a UdpStreamOutpur instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    // And calling UdpStreamOutput's bind method.
    auto ret = stream_output_->bind();

    // It shall succeed.
    EXPECT_TRUE(ret.has_value());
}

TEST_F(UdpStreamOutputFixture, SendMethodShallFailIfSendmmsgFailed)
{
    // When mocking socket's sendmmsg to return error.
    EXPECT_CALL(*sock_mock_, sendmmsg(_, _, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));

    struct mmsghdr mmsg_hdr_array[4UL];
    std::uint32_t size = 0UL;

    // And instantiating a UdpStreamOutpur instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    // And calling UdpStreamOutput's send method.
    score::cpp::span<mmsghdr> mmsg_span(mmsg_hdr_array, size);
    auto ret = stream_output_->send(mmsg_span);

    // It shall fail.
    EXPECT_FALSE(ret.has_value());
}

TEST_F(UdpStreamOutputFixture, SendMethodShallSucceedIfSendmmsgSucceeded)
{
    // When mocking socket's sendmmsg to return error.
    EXPECT_CALL(*sock_mock_, sendmmsg(_, _, _, _))
        .Times(Exactly(1))
        .WillOnce([]() -> score::cpp::expected<std::int32_t, score::os::Error> {
            // Return any value except -1 means success sending, also, it is not possible to return -1
            // regarding SocketImpl::sendmmsg implementation.
            return 5;
        });

    struct mmsghdr mmsg_hdr_array[4UL];
    std::uint32_t size = 0UL;

    // And instantiating a UdpStreamOutpur instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    // And calling UdpStreamOutput's send method.
    score::cpp::span<mmsghdr> mmsg_span(mmsg_hdr_array, size);
    auto ret = stream_output_->send(mmsg_span);

    // It shall succeed.
    EXPECT_TRUE(ret.has_value());
}

TEST_F(UdpStreamOutputFixture, SendMethodShallSucceedIfSendmmsgSucceededWithMmsg_spanNotEmpty)
{
    // When mocking socket's sendmmsg to return error.
    EXPECT_CALL(*sock_mock_, sendmmsg(_, _, _, _))
        .Times(Exactly(1))
        .WillOnce([]() -> score::cpp::expected<std::int32_t, score::os::Error> {
            // Return any value except -1 means success sending, also, it is not possible to return -1
            // regarding SocketImpl::sendmmsg implementation.
            return 5;
        });

    struct mmsghdr mmsg_hdr_array[4UL];
    std::uint32_t size = 1UL;

    // And instantiating a UdpStreamOutpur instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    // And calling UdpStreamOutput's send method.
    score::cpp::span<mmsghdr> mmsg_span(mmsg_hdr_array, size);
    auto ret = stream_output_->send(mmsg_span);

    // It shall succeed.
    EXPECT_TRUE(ret.has_value());
}

TEST_F(UdpStreamOutputFixture, SendMethodShallSucceedIfSendmsgSucceeded)
{
    // When mocking socket's sendmsg to return error.
    EXPECT_CALL(*sock_mock_, sendmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno())));

    iovec io_vec[4UL];
    std::uint32_t size = 0UL;

    // And instantiating a UdpStreamOutpur instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    // And calling UdpStreamOutput's send method.
    auto ret = stream_output_->send(io_vec, size);

    // It shall fail.
    EXPECT_FALSE(ret.has_value());
}

TEST_F(UdpStreamOutputFixture, SendMethodShallFailIfSendmsgFailed)
{
    // When mocking socket's sendmsg to return error.
    EXPECT_CALL(*sock_mock_, sendmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce([]() -> score::cpp::expected<ssize_t, score::os::Error> {
            // Return any value except -1 means success sending, also, it is not possible to return -1
            // regarding SocketImpl::sendmsg implementation.
            return 5;
        });

    iovec io_vec[4UL];
    std::uint32_t size = 0UL;

    // And instantiating a UdpStreamOutpur instance.
    stream_output_ = std::make_unique<UdpStreamOutput>(addr_, port_, multicast_interface_, std::move(sock_mock_));

    // And calling UdpStreamOutput's send method.
    auto ret = stream_output_->send(io_vec, size);

    // It shall succeed.
    EXPECT_TRUE(ret.has_value());
}

}  // namespace
}  // namespace dltserver
}  // namespace logging
}  // namespace score
