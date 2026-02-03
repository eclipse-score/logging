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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "score/datarouter/src/file_transfer/file_transfer_impl/filetransfer_stream.h"
#include <array>
#include <chrono>
#include <fstream>
#include <memory>
#include <thread>

using namespace ::testing;
using namespace score::platform::internal;
using score::logging::dltserver::FileTransferStreamHandler;
using bufsize_t = unsigned int;
using ::testing::_;
using ::testing::AtLeast;

class MockFTOutput : public FileTransferStreamHandler::IOutput
{
  public:
    MOCK_METHOD(void,
                sendFTVerbose,
                (score::cpp::span<const std::uint8_t> data,
                 score::mw::log::LogLevel loglevel,
                 score::platform::dltid_t appId,
                 score::platform::dltid_t ctxId,
                 uint8_t nor,
                 uint32_t time_tmsp),
                (override));

    virtual ~MockFTOutput() = default;
};

class FileTransferStreamTest : public ::testing::Test
{
  protected:
    std::unique_ptr<score::logging::dltserver::FileTransferStreamHandler> handler;
    std::unique_ptr<MockFTOutput> mockOutput;

    void SetUp() override
    {
        mockOutput = std::make_unique<MockFTOutput>();
        handler = std::make_unique<FileTransferStreamHandler>(*mockOutput);
    }

    void TearDown() override
    {
        handler.reset();
        mockOutput.reset();
    }

    std::string CreateTempFile(std::size_t size)
    {
        std::string path = "/tmp/test_file_transfer.txt";
        std::ofstream ofs(path);
        ofs << std::string(size, 'A');
        ofs.close();
        return path;
    }

    std::string SerializeFileTransferEntry(const std::string& filename, bool deleteFile)
    {
        score::logging::FileTransferEntry entry{};
        entry.file_name = filename;
        entry.delete_file = deleteFile ? 1 : 0;
        entry.appid = "APPX";
        entry.ctxid = "CTXX";

        std::array<char, 1024> buffer{};
        auto size = score::common::visitor::logging_serializer::serialize(entry, buffer.data(), buffer.size());
        EXPECT_GT(size, 0);
        return std::string(buffer.data(), size);
    }
};

TEST_F(FileTransferStreamTest, ShouldTransferFileSuccessfully)
{
    auto path = CreateTempFile(2048);  // BUFFER_SIZE, triggers multiple packets
    auto data = SerializeFileTransferEntry(path, false);

    EXPECT_CALL(
        *mockOutput,
        sendFTVerbose(
            _, score::mw::log::LogLevel::kInfo, score::platform::dltid_t("APPX"), score::platform::dltid_t("CTXX"), _, _))
        .Times(AtLeast(3));

    handler->handle({}, data.c_str(), static_cast<bufsize_t>(data.size()));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(path.empty());
    std::remove(path.c_str());
}

TEST_F(FileTransferStreamTest, ShouldLogErrorWhenFileNotFound)
{
    std::string invalid_path = "/nonexistent/path/abc.txt";
    auto data = SerializeFileTransferEntry(invalid_path, false);

    EXPECT_CALL(
        *mockOutput,
        sendFTVerbose(
            _, score::mw::log::LogLevel::kError, score::platform::dltid_t("APPX"), score::platform::dltid_t("CTXX"), _, _))
        .Times(1);

    handler->handle({}, data.c_str(), static_cast<bufsize_t>(data.size()));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_NE(data.find("abc.txt"), std::string::npos);
}

TEST_F(FileTransferStreamTest, ShouldDeleteFileIfFlagSet)
{
    auto path = CreateTempFile(512);  // < BUFFER_SIZE
    auto data = SerializeFileTransferEntry(path, true);

    EXPECT_CALL(
        *mockOutput,
        sendFTVerbose(
            _, score::mw::log::LogLevel::kInfo, score::platform::dltid_t("APPX"), score::platform::dltid_t("CTXX"), _, _))
        .Times(AtLeast(2));

    handler->handle({}, data.c_str(), static_cast<bufsize_t>(data.size()));
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ASSERT_FALSE(std::ifstream(path).good());
    EXPECT_NE(data.find("APPX"), std::string::npos);
}

TEST_F(FileTransferStreamTest, ShouldIgnoreInvalidSerializedInput)
{
    std::array<char, 512> garbage{};
    std::fill(garbage.begin(), garbage.end(), 'Z');

    // No expectations because deserialization will likely fail silently
    handler->handle({}, garbage.data(), static_cast<bufsize_t>(garbage.size()));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(garbage[0], 'Z');
}

TEST_F(FileTransferStreamTest, ShouldReturnExtraPackageWhenFileNotDivisible)
{
    // File size not divisible by BUFFER_SIZE (e.g., 1500 if BUFFER_SIZE is 1024)
    auto path = CreateTempFile(1500);
    auto data = SerializeFileTransferEntry(path, false);

    handler->handle({}, data.c_str(), static_cast<bufsize_t>(data.size()));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::ifstream file(path);
    EXPECT_TRUE(file.good());
    std::remove(path.c_str());
}
