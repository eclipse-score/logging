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

#include "score/os/mocklib/socketmock.h"
#include "unix_domain/unix_domain_common.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

namespace score
{
namespace platform
{
namespace internal
{

using namespace testing;
class UnixDomainCommonTestFixture : public ::testing::Test
{
  public:
    std::unique_ptr<score::os::SocketMock> sock_mock;
    void SetUp() override
    {
        sock_mock = std::make_unique<score::os::SocketMock>();
        score::os::Socket::set_testing_instance(*sock_mock);
    }
    void TearDown() override
    {

        score::os::Socket::restore_instance();
    }
};

TEST_F(UnixDomainCommonTestFixture, Constructor_unix_domain_common)
{
    UnixDomainSockAddr addr("datarouter_socket", true);
    EXPECT_EQ(std::string(addr.GetAddressString()), "datarouter_socket");
    EXPECT_EQ(addr.IsAbstract(), true);
}

TEST_F(UnixDomainCommonTestFixture, SendAncillaryDataOverSocket_Succeeded)
{
    score::cpp::span<std::uint8_t> data{};
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _)).Times(Exactly(1)).WillOnce(Return(1));
    SendAncillaryDataOverSocket(5, data);
}

TEST_F(UnixDomainCommonTestFixture, SendAncillaryDataOverSocket_SendmsgReturnError)
{
    score::cpp::span<std::uint8_t> data{};
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))));
    SendAncillaryDataOverSocket(5, data);
}

TEST_F(UnixDomainCommonTestFixture, SendAncillaryDataOverSocket_FailedSendMsgFailedWithWrongValue)
{
    std::stringstream buffer;
    std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());
    score::cpp::span<std::uint8_t> data{};
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO))));
    SendAncillaryDataOverSocket(5, data);
    std::cerr.rdbuf(old_cerr);
    std::string output = buffer.str();
    EXPECT_THAT(output, testing::HasSubstr("sendmsg: Error reported with errno"));
}

TEST_F(UnixDomainCommonTestFixture, SendAncillaryDataOverSocket_SendmsgReturnError_Twice)
{
    score::cpp::span<std::uint8_t> data{};
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _))
        .Times(Exactly(2))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))));

    SendAncillaryDataOverSocket(5, data);
    SendAncillaryDataOverSocket(5, data);
}

TEST_F(UnixDomainCommonTestFixture, send_socket_message_WithFileHandleLessThanZero)
{
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _)).Times(Exactly(1)).WillOnce(Return(1));
    SendSocketMessage(5, "Request", -1);
}

TEST_F(UnixDomainCommonTestFixture, send_socket_message_RetuenError)
{
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))));
    SendSocketMessage(5, "Request", 5);
}

TEST_F(UnixDomainCommonTestFixture, send_socket_message_RetuenError_Twice)
{
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _))
        .Times(Exactly(2))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))));
    SendSocketMessage(5, "Request", 5);
    SendSocketMessage(5, "Request", 5);
}

TEST_F(UnixDomainCommonTestFixture, send_socket_message_Succeeded)
{
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _)).Times(Exactly(1)).WillOnce(Return(1));
    SendSocketMessage(5, "Request", 5);
}

TEST_F(UnixDomainCommonTestFixture, send_socket_message_Failed)
{
    std::stringstream buffer;
    std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());
    EXPECT_CALL(*sock_mock, sendmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO))));
    SendSocketMessage(5, "Request", 5);
    std::cerr.rdbuf(old_cerr);
    std::string output = buffer.str();
    EXPECT_THAT(output, testing::HasSubstr("sendmsg: Error reported with errno"));
}

TEST_F(UnixDomainCommonTestFixture, recv_socket_message_FirstRecvReturnError)
{
    EXPECT_CALL(*sock_mock, recvmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))));
    auto ret = RecvSocketMessage(5);

    EXPECT_EQ(ret.has_value(), true);

    EXPECT_CALL(*sock_mock, recvmsg(_, _, _))
        .Times(Exactly(1))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINTR))));
    ret = RecvSocketMessage(5);

    EXPECT_EQ(ret.has_value(), true);
}

TEST_F(UnixDomainCommonTestFixture, recv_socket_message_FirstRecvFailedWithWrongHeaderSize)
{
    std::stringstream buffer;
    std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());

    EXPECT_CALL(*sock_mock, recvmsg(_, _, _)).Times(Exactly(1)).WillOnce(Return(2));
    auto ret = RecvSocketMessage(5);

    std::cerr.rdbuf(old_cerr);
    std::string output = buffer.str();
    EXPECT_THAT(output, testing::HasSubstr("Unix Domain Socket communication is corrupted!"));
    EXPECT_EQ(ret.has_value(), false);
}

TEST_F(UnixDomainCommonTestFixture, recv_socket_message_FirstRecvSucceededandSecondRecvReturnError)
{
    EXPECT_CALL(*sock_mock, recvmsg(_, _, _))
        .Times(Exactly(2))
        .WillOnce(Return(8))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EAGAIN))));
    auto ret = RecvSocketMessage(5);

    EXPECT_EQ(ret.has_value(), true);
    EXPECT_EQ(ret.value(), std::string{});
}

TEST_F(UnixDomainCommonTestFixture, recv_socket_message_FirstRecvSucceededandSecondSucceededWithZeroValue)
{
    EXPECT_CALL(*sock_mock, recvmsg(_, _, _)).Times(Exactly(2)).WillOnce(Return(8)).WillOnce(Return(0));
    auto ret = RecvSocketMessage(5);

    EXPECT_EQ(ret.has_value(), false);
}

TEST_F(UnixDomainCommonTestFixture, recv_socket_message_FirstRecvSucceededandSecondSucceeded)
{
    EXPECT_CALL(*sock_mock, recvmsg(_, _, _)).Times(Exactly(2)).WillOnce(Return(8)).WillOnce(Return(1));
    auto ret = RecvSocketMessage(5);
    EXPECT_EQ(ret.has_value(), true);
}

TEST_F(UnixDomainCommonTestFixture, recv_socket_message_SocketMessangerHeaderForTypekSharedMemoryFileHandle)
{
    std::stringstream buffer;
    std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());
    auto my_lambda = [](const score::cpp::span<std::uint8_t>) -> score::cpp::optional<SharedMemoryFileHandle> {
        return 10;
    };
    score::cpp::optional<SharedMemoryFileHandle> file_handle{0};
    score::cpp::optional<std::int32_t> peer_pid{};
    EXPECT_CALL(*sock_mock, recvmsg(_, _, _))
        .Times(Exactly(2))
        .WillOnce(::testing::Invoke([&](const std::int32_t, msghdr* const message, const score::os::Socket::MessageFlag) {
            reinterpret_cast<SocketMessangerHeader*>(reinterpret_cast<iovec*>(message->msg_iov)->iov_base)->type =
                MessageType::kSharedMemoryFileHandle;
            return 8;
        }))
        .WillOnce(Return(1));

    auto ret = RecvSocketMessage(5, file_handle, peer_pid, my_lambda);

    EXPECT_EQ(ret.has_value(), true);
    std::cerr.rdbuf(old_cerr);
    std::string output = buffer.str();
    EXPECT_THAT(output, testing::HasSubstr("Overwriting file descriptor handle may lead to leaks"));
}

TEST_F(UnixDomainCommonTestFixture, recv_socket_message_SocketMessangerHeaderForWrongType)
{
    std::stringstream buffer;
    std::streambuf* old_cerr = std::cerr.rdbuf(buffer.rdbuf());

    EXPECT_CALL(*sock_mock, recvmsg(_, _, _))
        .Times(Exactly(2))
        .WillOnce(::testing::Invoke([&](const std::int32_t, msghdr* const message, const score::os::Socket::MessageFlag) {
            reinterpret_cast<SocketMessangerHeader*>(reinterpret_cast<iovec*>(message->msg_iov)->iov_base)->type =
                static_cast<MessageType>(2);
            return 8;
        }))
        .WillOnce(Return(1));

    auto ret = RecvSocketMessage(5);

    EXPECT_EQ(ret.has_value(), false);
    std::cerr.rdbuf(old_cerr);
    std::string output = buffer.str();
    EXPECT_THAT(output, testing::HasSubstr("UnixDomain: recvmsg Error"));
}

}  // namespace internal
}  // namespace platform
}  // namespace score
