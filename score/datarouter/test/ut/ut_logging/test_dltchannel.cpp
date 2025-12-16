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

#include "daemon/dlt_log_channel.h"
#include "daemon/udp_stream_output.h"

#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

namespace test
{

using namespace score::logging::dltserver;
using namespace score::platform::internal;
using LogEntry_t = ::score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection;

struct Logger
{
    std::string stream;
    std::stringstream LogDebug()
    {
        return std::stringstream{stream};
    }
    std::stringstream LogInfo()
    {
        return std::stringstream{stream};
    }
    std::stringstream LogWarn()
    {
        return std::stringstream{stream};
    }
    std::stringstream LogError()
    {
        return std::stringstream{stream};
    }
};

class DltChannelTest : public ::testing::Test
{
  public:
    void SetUp() override {}
    void TearDown() override {}
    DltChannelTest() = default;
    ~DltChannelTest() = default;

  protected:
    // constants from DltLogChannel class
    static constexpr uint32_t kIpv4HeaderWithoutOptions = 20UL;
    static constexpr uint32_t kUdpHeader = 8UL;
    static constexpr uint32_t kMtu = 1500UL;
    static constexpr uint32_t UDP_MAX_PAYLOAD = kMtu - (kIpv4HeaderWithoutOptions + kUdpHeader);

  protected:
    // predefined verbose and non-verbose messages for tests
    const std::array<uint8_t, 8> msg1_{1, 2, 3, 4, 5, 6, 7, 8};
    const std::array<uint8_t, 8> msg2_{11, 12, 13, 14, 15, 16, 17, 18};

    const score::mw::log::config::NvMsgDescriptor nv_desc1_{1234U,
                                                          score::mw::log::detail::LoggingIdentifier{"APP0"},
                                                          score::mw::log::detail::LoggingIdentifier{"CTX0"},
                                                          score::mw::log::LogLevel::kOff};
    const score::mw::log::config::NvMsgDescriptor nv_desc2_{1235U,
                                                          score::mw::log::detail::LoggingIdentifier{"APP1"},
                                                          score::mw::log::detail::LoggingIdentifier{"CTX0"},
                                                          score::mw::log::LogLevel::kOff};

    const LogEntry_t verbose_entry1_{score::mw::log::detail::LoggingIdentifier{"APP0"},
                                     score::mw::log::detail::LoggingIdentifier{"CTX0"},
                                     {score::cpp::span<const uint8_t>{msg1_}},
                                     1,
                                     score::mw::log::LogLevel::kOff};
    const LogEntry_t verbose_entry2_{score::mw::log::detail::LoggingIdentifier{"APP1"},
                                     score::mw::log::detail::LoggingIdentifier{"CTX0"},
                                     {score::cpp::span<const uint8_t>{msg2_}},
                                     1,
                                     score::mw::log::LogLevel::kOff};
};

TEST_F(DltChannelTest, WhenCreatedDefault)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");
}

TEST_F(DltChannelTest, WhenSendingNonverboseTwice)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    mmsghdr mmsghdr_data;
    score::cpp::span<mmsghdr> mmsg_span(&mmsghdr_data, 1);

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).WillOnce(DoAll(SaveArg<1>(&mmsg_span), Return(1)));

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    dltChannel.sendNonVerbose(nv_desc1_, 1U, msg1_.data(), msg1_.size());
    dltChannel.sendNonVerbose(nv_desc2_, 2U, msg2_.data(), msg2_.size());
    dltChannel.flush();

    // check 2 non-verbose messages on UDP level (check by size)
    const auto sent_messages_count = 2;
    ASSERT_EQ(mmsg_span.subspan(0, 1).front().msg_hdr.msg_iovlen, 1);
    EXPECT_EQ(mmsg_span.subspan(0, 1).front().msg_hdr.msg_iov[0].iov_len,
              ((sizeof(DltNvHeaderWithMsgid) + 8) * sent_messages_count));

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenSendingVerboseTwice)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    mmsghdr mmsghdr_data;
    score::cpp::span<mmsghdr> mmsg_span(&mmsghdr_data, 1);

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).WillOnce(DoAll(SaveArg<1>(&mmsg_span), Return(1)));

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    dltChannel.sendVerbose(1U, verbose_entry1_);
    dltChannel.sendVerbose(2U, verbose_entry2_);
    dltChannel.flush();

    // check 2 verbose messages on UDP level (check by size)
    const auto sent_messages_count = 2;
    ASSERT_EQ(mmsg_span.subspan(0, 1).front().msg_hdr.msg_iovlen, 1);
    EXPECT_EQ(mmsg_span.subspan(0, 1).front().msg_hdr.msg_iov[0].iov_len,
              ((sizeof(DltVerboseHeader) + 8) * sent_messages_count));

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenSendingNvVNv)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>()))
        .WillOnce(DoAll(Invoke([](UdpStreamOutput*, score::cpp::span<mmsghdr> data_span) {
                            const auto expected_data_size_1 = sizeof(DltNvHeaderWithMsgid) + 8;
                            const auto expected_data_size_2 = sizeof(DltVerboseHeader) + 8;
                            const auto expected_data_size_3 = sizeof(DltNvHeaderWithMsgid) + 8;

                            // check data size in all 3 messages
                            ASSERT_EQ(data_span.size(), 3);
                            EXPECT_EQ(data_span.subspan(0, 1).front().msg_hdr.msg_iov[0].iov_len, expected_data_size_1);
                            EXPECT_EQ(data_span.subspan(1, 1).front().msg_hdr.msg_iov[0].iov_len, expected_data_size_2);
                            EXPECT_EQ(data_span.subspan(2, 1).front().msg_hdr.msg_iov[0].iov_len, expected_data_size_3);
                        }),
                        Return(1)));

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    dltChannel.sendNonVerbose(nv_desc1_, 1U, msg1_.data(), msg1_.size());
    dltChannel.sendVerbose(2U, verbose_entry1_);
    dltChannel.sendNonVerbose(nv_desc2_, 3U, msg2_.data(), msg2_.size());
    dltChannel.flush();

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, TestSendUdpBufferingNonVerbose)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    // all data should be buffered, without send to socket
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).Times(0);
    // send a lot of data to fill DltLogChannel::prebuf_data_ to force using next buffer
    const auto expected_prebuf_size = UDP_MAX_PAYLOAD;
    const auto length_of_one_message = (sizeof(DltNvHeaderWithMsgid) + msg1_.size());
    const auto message_count_to_fill_prebuf = expected_prebuf_size / length_of_one_message;
    for (size_t i = 0; i < message_count_to_fill_prebuf; ++i)
    {
        dltChannel.sendNonVerbose(nv_desc1_, static_cast<uint32_t>(i + 1), msg1_.data(), msg1_.size());
    }
    Mock::VerifyAndClearExpectations(&outputs);

    // send another packet. it should be put to another buffer, because first buffer is already full
    // all data still should be buffered, no calls to socket
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).Times(0);
    dltChannel.sendNonVerbose(nv_desc2_, 1U, msg2_.data(), msg2_.size());
    Mock::VerifyAndClearExpectations(&outputs);

    // flush data and send it to socket
    EXPECT_CALL(outputs, destruct(_)).Times(1);
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>()))
        .WillOnce(DoAll(Invoke([length_of_one_message, message_count_to_fill_prebuf](UdpStreamOutput*,
                                                                                     score::cpp::span<mmsghdr> data_span) {
                            // first buffer if full
                            const auto expected_data_size_1 = length_of_one_message * message_count_to_fill_prebuf;
                            // second buffer should contain only one message
                            const auto expected_data_size_2 = length_of_one_message;

                            // check data size for all buffers
                            ASSERT_EQ(data_span.size(), 2);
                            EXPECT_EQ(data_span.subspan(0, 1).front().msg_hdr.msg_iov[0].iov_len, expected_data_size_1);
                            EXPECT_EQ(data_span.subspan(1, 1).front().msg_hdr.msg_iov[0].iov_len, expected_data_size_2);
                        }),
                        Return(1)));

    dltChannel.flush();

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, TestSendUdpBufferingForVerbose)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    // Ensure data should be buffered without sending it
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).Times(0);

    // Fill the prebuffer until it's full but still below UDP_MAX_PAYLOAD
    const auto expected_prebuf_size = UDP_MAX_PAYLOAD;
    const auto length_of_one_message = (sizeof(DltVerboseHeader) + msg1_.size());
    const auto message_count_to_fill_prebuf = expected_prebuf_size / length_of_one_message;

    for (size_t i = 0; i < message_count_to_fill_prebuf; ++i)
    {
        dltChannel.sendVerbose(1U, verbose_entry1_);
    }

    Mock::VerifyAndClearExpectations(&outputs);

    // Send another verbose message, which should be placed into a new buffer
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).Times(0);
    dltChannel.sendVerbose(2U, verbose_entry2_);
    Mock::VerifyAndClearExpectations(&outputs);

    // Flush data and validate it is sent in two chunks (one full buffer + one extra)
    EXPECT_CALL(outputs, destruct(_)).Times(1);
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>()))
        .WillOnce(DoAll(Invoke([length_of_one_message, message_count_to_fill_prebuf](UdpStreamOutput*,
                                                                                     score::cpp::span<mmsghdr> data_span) {
                            // First buffer is full
                            const auto expected_data_size_1 = length_of_one_message * message_count_to_fill_prebuf;
                            // Second buffer contains one additional message
                            const auto expected_data_size_2 = length_of_one_message;

                            // Validate buffer sizes
                            ASSERT_EQ(data_span.size(), 2);
                            EXPECT_EQ(data_span.subspan(0, 1).front().msg_hdr.msg_iov[0].iov_len, expected_data_size_1);
                            EXPECT_EQ(data_span.subspan(1, 1).front().msg_hdr.msg_iov[0].iov_len, expected_data_size_2);
                        }),
                        Return(1)));

    dltChannel.flush();

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenSendFailsWithOnlyVerboseMessages)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    mmsghdr mmsghdr_data;
    score::cpp::span<mmsghdr> mmsg_span(&mmsghdr_data, 1);

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>()))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO))));

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    dltChannel.sendVerbose(1U, verbose_entry1_);
    dltChannel.sendVerbose(2U, verbose_entry2_);

    dltChannel.flush();
    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenSendFailsWithOnlyNonVerboseMessages)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;
    mmsghdr mmsghdr_data;
    score::cpp::span<mmsghdr> mmsg_span(&mmsghdr_data, 1);

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    ;
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
    EXPECT_CALL(outputs, destruct(_)).Times(1);
    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>()))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO))));

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    // Add only non-verbose messages
    dltChannel.sendNonVerbose(nv_desc1_, 1U, msg1_.data(), msg1_.size());
    dltChannel.sendNonVerbose(nv_desc2_, 2U, msg2_.data(), msg2_.size());

    dltChannel.flush();

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenSendingLargeMessage_GoesToElse)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    EXPECT_CALL(outputs, send(_, A<const iovec*>(), A<size_t>()))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    // Create a single large message bigger than UDP_MAX_PAYLOAD
    std::vector<uint8_t> large_msg(UDP_MAX_PAYLOAD + 100, 0xAA);  // Large message (slightly bigger than the max)

    dltChannel.sendNonVerbose(nv_desc1_, 1U, large_msg.data(), large_msg.size());  // Send oversized message

    dltChannel.flush();

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenSendFailsWithLargeVerboseMessage)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    // Expect the constructor and binding to be called
    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    // Make sendmsg return an error (simulate failure when sending large verbose message)
    EXPECT_CALL(outputs, send(_, A<const iovec*>(), A<size_t>()))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EIO))));

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    // Send a large verbose message to trigger the failure condition
    std::vector<uint8_t> large_payload(UDP_MAX_PAYLOAD + 1, 0xAB);  // Large payload
    LogEntry_t large_verbose_entry{score::mw::log::detail::LoggingIdentifier{"APP0"},
                                   score::mw::log::detail::LoggingIdentifier{"CTX0"},
                                   {score::cpp::span<const uint8_t>{large_payload}},
                                   1,
                                   score::mw::log::LogLevel::kOff};

    dltChannel.sendVerbose(1U, large_verbose_entry);  // Should go to "sendmsg" instead of "sendmmsg"
    dltChannel.flush();

    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenSendingFTVerboseHitsSleepCondition)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    using namespace score::mw::log;

    score::cpp::span<const uint8_t> msg_data(msg1_.data(), msg1_.size());
    LogLevel log_level = LogLevel::kOff;  // Fixed namespace usage
    dltid_t appId{"APP0"};
    dltid_t ctxId{"CTX0"};
    uint8_t nor = 1;
    uint32_t timestamp = 1;

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);
    EXPECT_CALL(outputs, send(_, A<const iovec*>(), A<size_t>()))
        .WillRepeatedly(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));

    DltLogChannel dltChannel(dltid_t{"CHN0"}, log_level, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    // Trigger iteration counter to reach kBurstFileTransferControlCount threshold
    constexpr auto kTestBurstFileTransferControlCount = 5UL;
    for (size_t i = 0; i < kTestBurstFileTransferControlCount; ++i)
    {
        dltChannel.sendFTVerbose(msg_data, log_level, appId, ctxId, nor, timestamp);
    }
    Logger logger;
    dltChannel.show_stats(logger);
}

TEST_F(DltChannelTest, WhenLogLevelExceedsThreshold_Verbose)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    // Set a high log level that should be ignored
    LogEntry_t high_log_entry = verbose_entry1_;
    high_log_entry.log_level = score::mw::log::LogLevel::kFatal;

    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).Times(0);

    dltChannel.sendVerbose(1U, high_log_entry);
}

TEST_F(DltChannelTest, WhenNonVerboseLogLevelExceedsThreshold)
{
    testing::StrictMock<UdpStreamOutput::Tester> outputs;
    UdpStreamOutput::Tester::instance() = &outputs;

    EXPECT_CALL(outputs, construct(_, nullptr, 3490U, Eq(std::string("")))).Times(1);
    EXPECT_CALL(outputs, bind(_, nullptr, 3491U)).Times(1);
    EXPECT_CALL(outputs, destruct(_)).Times(1);

    DltLogChannel dltChannel(
        dltid_t{"CHN0"}, score::mw::log::LogLevel::kOff, dltid_t{"ECU0"}, nullptr, 3491U, nullptr, 3490U, "");

    auto high_log_desc = nv_desc1_;
    high_log_desc.SetLogLevel(score::mw::log::LogLevel::kFatal);

    EXPECT_CALL(outputs, send(_, A<score::cpp::span<mmsghdr>>())).Times(0);
    dltChannel.sendNonVerbose(high_log_desc, 1U, msg1_.data(), msg1_.size());
}

}  //  namespace test
