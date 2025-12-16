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

#include "gtest/gtest.h"
#include "score/datarouter/include/dlt/dlt_protocol.h"
#include "score/datarouter/include/dlt/dltid.h"
#include "iostream"
#include <fstream>

const std::string FILE_NAME{"score/datarouter/test/ut/etc/dummy_file_transfer"};

TEST(DltProtocolTest, PackageFileHeaderShallReturnsNulloptIftheBufferSizeIsSmallerThanTheDataSize)
{
    const auto buffer_size{0};
    std::array<char, buffer_size> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};
    const std::string file_name{"any name"};
    const uint32_t fsize{0U};
    const std::string creation_date("any date");
    const uint32_t package_count{0U};

    auto result = PackageFileHeader(data_span, serial_number, file_name, fsize, creation_date, package_count);
    EXPECT_EQ(result, std::nullopt);
}

TEST(DltProtocolTest, PackageFileHeaderShallWriteHeaderCorrectlyWithCorrectData)
{
    const auto buffer_size{1024};
    std::array<char, buffer_size> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};
    const std::string file_name{"any name"};
    const uint32_t fsize{0U};
    const std::string creation_date("any date");
    const uint32_t package_count{0U};

    auto result = PackageFileHeader(data_span, serial_number, file_name, fsize, creation_date, package_count);
    EXPECT_TRUE(result.has_value());
}

TEST(DltProtocolTest, PackageFileDataShallReturnsNulloptIftheBufferSizeIsSmallerThanTheDataSize)
{
    const auto buffer_size{0};
    std::array<char, buffer_size> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};
    const uint32_t pkg_number{1U};

    FILE* file = fopen(FILE_NAME.c_str(), "rb");
    ASSERT_TRUE(file != nullptr) << "The file used in the unit test is missed! The file: " << FILE_NAME;

    auto result = PackageFileData(data_span, file, serial_number, pkg_number);
    EXPECT_EQ(result, std::nullopt);
}

TEST(DltProtocolTest, PackageFileDataShallWriteDataCorrectlyWithCorrectData)
{
    std::array<char, 4096> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};
    const uint32_t pkg_number{1U};

    FILE* file = fopen(FILE_NAME.c_str(), "rb");
    ASSERT_TRUE(file != nullptr) << "The file used in the unit test is missed! The file: " << FILE_NAME;

    auto result = PackageFileData(data_span, file, serial_number, pkg_number);
    EXPECT_TRUE(result.has_value());
}

// Kindly, check the code, inside the "PackageFileData" method for the reasons of disabling this test.
TEST(DltProtocolTest, DISABLED_PackageFileDataShallReturnsNulloptIfItWorkedOnAlreadyClosedFile)
{
    std::array<char, BUFFER_SIZE> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};
    const uint32_t pkg_number{0};

    FILE* file = fopen(FILE_NAME.c_str(), "rb");
    ASSERT_TRUE(file != nullptr) << "The file used in the unit test is missed! The file: " << FILE_NAME;
    fclose(file);  // Close the file immediately.

    auto result = PackageFileData(data_span, file, serial_number, pkg_number);
    EXPECT_EQ(result, std::nullopt);
}

TEST(DltProtocolTest, PackageFileEndShallReturnsNulloptIftheBufferSizeIsSmallerThanTheDataSize)
{
    const auto buffer_size{0};
    std::array<char, buffer_size> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};

    auto result = PackageFileEnd(data_span, serial_number);
    EXPECT_EQ(result, std::nullopt);
}

TEST(DltProtocolTest, PackageFileEndShallWriteHeaderCorrectlyWithCorrectData)
{
    const auto buffer_size{64};
    std::array<char, buffer_size> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};

    auto result = PackageFileEnd(data_span, serial_number);
    EXPECT_TRUE(result.has_value());
}

TEST(DltProtocolTest, PackageFileErrorShallReturnFLER_FILE_NORIfTheFilePathIsExist)
{
    std::array<char, BUFFER_SIZE> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    // Any error code.
    const int16_t error_code{DLT_FILETRANSFER_ERROR_FILE_DATA};
    const uint32_t serial_number{0U};
    const std::string file_name{FILE_NAME};
    const uint32_t file_size{0U};
    const std::string creation_date{"any date"};
    const uint32_t package_count{0U};
    const std::string& error_message = "";

    auto result = PackageFileError(
        data_span, error_code, serial_number, file_name, file_size, creation_date, package_count, error_message);
    // Index '1' to get the 'nor' from the tuple.
    EXPECT_EQ(std::get<1>(result.value()), FLER_FILE_NOR);
}

TEST(DltProtocolTest, PackageFileErrorShallReturnFLER_FILE_NORPlusOneIfTheFilePathExistAndTheErrorMessageIsNotEmpty)
{
    std::array<char, BUFFER_SIZE> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    // Any error code.
    const int16_t error_code{DLT_FILETRANSFER_ERROR_FILE_DATA};
    const uint32_t serial_number{0U};
    const std::string file_name{FILE_NAME};
    const uint32_t file_size{0U};
    const std::string creation_date{"any date"};
    const uint32_t package_count{0U};
    const std::string& error_message = "any error message";

    auto result = PackageFileError(
        data_span, error_code, serial_number, file_name, file_size, creation_date, package_count, error_message);
    // Index '1' to get the 'nor' from the tuple.
    EXPECT_EQ(std::get<1>(result.value()), FLER_FILE_NOR + 1);
}

TEST(DltProtocolTest, PackageFileErrorShallReturnFLER_NO_FILE_NORIfTheFilePathIsExist)
{
    auto temp_ = errno;
    errno = ENOENT;
    std::array<char, BUFFER_SIZE> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    // Any error code.
    const int16_t error_code{DLT_FILETRANSFER_ERROR_FILE_DATA};
    const uint32_t serial_number{0U};
    const std::string file_name{FILE_NAME};
    const uint32_t file_size{0U};
    const std::string creation_date{"any date"};
    const uint32_t package_count{0U};
    const std::string& error_message = "";

    auto result = PackageFileError(
        data_span, error_code, serial_number, file_name, file_size, creation_date, package_count, error_message);
    // Index '1' to get the 'nor' from the tuple.
    errno = temp_;
    EXPECT_EQ(std::get<1>(result.value()), FLER_NO_FILE_NOR);
}

TEST(DltProtocolTest, PackageFileInformationShallReturnNulloptDueToBufferTooSmall)
{
    const auto buffer_size{64};
    std::array<char, buffer_size> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};
    const std::string file_name{"any name"};
    const uint32_t fsize{0U};
    const std::string creation_date("any date");
    const uint32_t package_count{0U};

    auto result = PackageFileInformation(data_span, serial_number, file_name, fsize, creation_date, package_count);
    EXPECT_EQ(result, std::nullopt);
}

TEST(DltProtocolTest, PackageFileInformationShallReturnDataOnCorrectSize)
{
    const auto buffer_size{1024};
    std::array<char, buffer_size> buffer{};
    auto data_span = score::cpp::span<char>{buffer.data(), buffer.size()};
    const uint32_t serial_number{0U};
    const std::string file_name{"any name"};
    const uint32_t fsize{0U};
    const std::string creation_date("any date");
    const uint32_t package_count{0U};

    auto result = PackageFileInformation(data_span, serial_number, file_name, fsize, creation_date, package_count);
    EXPECT_TRUE(result.has_value());
}

#define FOUR_CHAR_STRING "four"

TEST(DltIdTest, TestInitializeInstanceWithEmptyConstructionInTheStack)
{
    // If it failed, the whole test proccess will fail.
    score::platform::dltid_t dlt_id{};
}

TEST(DltIdTest, TestTheSingleargumentCharPointerConstructionAndGetTheData)
{
    const char* char_pointer_str{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id{char_pointer_str};

    const char* get_data = dlt_id.data();

    // The size of dlt id is four.
    EXPECT_EQ(get_data[0], char_pointer_str[0]);
    EXPECT_EQ(get_data[1], char_pointer_str[1]);
    EXPECT_EQ(get_data[2], char_pointer_str[2]);
    EXPECT_EQ(get_data[3], char_pointer_str[3]);
}

TEST(DltIdTest, TestDltIdEqualityOperator)
{
    const char* char_pointer_str{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id{char_pointer_str};
    score::platform::dltid_t dlt_id_1{char_pointer_str};

    EXPECT_TRUE(dlt_id_1 == dlt_id);
}

TEST(DltIdTest, TestTheSingleargumentStringConstruction)
{
    // If it failed, the whole test proccess will fail.
    const std::string str{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id{str};
}

TEST(DltIdTest, TestTheSingleargumentStringViewConstruction)
{
    // If it failed, the whole test proccess will fail.
    const score::cpp::string_view str_view{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id{str_view};
}

TEST(DltIdTest, DltIdSizeShouldBeEqualToFour)
{
    const std::string str{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id{str};

    EXPECT_EQ(dlt_id.size(), score::platform::dltid_t::kSize);
}

TEST(DltIdTest, TestDltIdAssignOperator)
{
    score::platform::dltid_t dlt_id{};

    auto dlt_id_1 = dlt_id;
    EXPECT_TRUE(dlt_id_1 == dlt_id);
}

TEST(DltIdTest, TestDltIdStringOperator)
{
    const std::string str{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id{str};

    // Casting to std::string.
    auto dlt_id_string = std::string(dlt_id);

    EXPECT_EQ(dlt_id_string, FOUR_CHAR_STRING);
    EXPECT_EQ(dlt_id_string.size(), std::strlen(FOUR_CHAR_STRING));
}

TEST(DltIdTest, TestDltIdAssignmentOperator)
{
    const std::string str{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id;
    dlt_id = str;

    // Casting to std::string.
    auto dlt_id_string = std::string(dlt_id);

    EXPECT_EQ(dlt_id_string, FOUR_CHAR_STRING);
    EXPECT_EQ(dlt_id_string.size(), std::strlen(FOUR_CHAR_STRING));
}

TEST(DltIdTest, TestHashStruct)
{
    const std::string str{FOUR_CHAR_STRING};
    score::platform::dltid_t dlt_id{str};

    std::hash<score::platform::dltid_t> hash_instance;
    std::size_t dlt_id_value = hash_instance(dlt_id);  // operator()

    EXPECT_EQ(dlt_id_value, dlt_id.value);
}
