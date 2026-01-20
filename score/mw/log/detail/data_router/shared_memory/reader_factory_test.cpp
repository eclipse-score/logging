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

#include "score/mw/log/detail/data_router/shared_memory/reader_factory_impl.h"

#include "score/os/mocklib/mman_mock.h"
#include "score/os/mocklib/stat_mock.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
namespace
{

using ::testing::_;
using ::testing::Return;

//  Testing:
//  score::cpp::optional<SharedMemoryReader> ReaderFactory::Create(const std::int32_t file_handle, const pid_t expected_pid)

static constexpr auto kDefaultRingSize = 1024UL;
static constexpr auto kLinearBufferSize = 1024UL / 2UL;
static constexpr auto kSharedSize = kDefaultRingSize + sizeof(SharedData);
static constexpr int kFileHandle{15};
static constexpr pid_t kExpectedPid{0x137};
static constexpr auto kMmapOffset = 0;

class ReaderFactoryFixture : public ::testing::Test
{
  public:
    ReaderFactoryFixture() : shared_data_(*(::new(&buffer_) SharedData))
    {
        shared_data_.linear_buffer_1_offset = sizeof(SharedData);
        shared_data_.linear_buffer_2_offset = sizeof(SharedData) + kLinearBufferSize;
        shared_data_.producer_pid = kExpectedPid;
    }
    ~ReaderFactoryFixture()
    {
        reinterpret_cast<SharedData*>(&buffer_)->~SharedData();
    }

    score::cpp::pmr::memory_resource* memory_resource_ = score::cpp::pmr::get_default_resource();
    score::cpp::pmr::unique_ptr<score::os::MmanMock> mman_mock_pmr = score::cpp::pmr::make_unique<score::os::MmanMock>(memory_resource_);
    score::cpp::pmr::unique_ptr<score::os::StatMock> stat_mock_pmr = score::cpp::pmr::make_unique<score::os::StatMock>(memory_resource_);
    score::os::MmanMock* mman_mock_ = mman_mock_pmr.get();
    score::os::StatMock* stat_mock_ = stat_mock_pmr.get();
    std::aligned_storage_t<kSharedSize, alignof(SharedData)> buffer_;
    SharedData& shared_data_;
    ReaderFactoryImpl factory_{std::move(mman_mock_pmr), std::move(stat_mock_pmr)};
};

TEST_F(ReaderFactoryFixture, FailingCallToFstatShallResultInEmptyOptional)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Reader creation shall fail in case of failing invocation of fstat.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    //  We expect mmap not to be called irregardless of arguments
    EXPECT_CALL(*mman_mock_, mmap(_, _, _, _, _, _)).Times(0);

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_EQ(result, nullptr);
}

TEST_F(ReaderFactoryFixture, FstatInvalidReturnShallResultInEmptyOptional)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Reader creation shall fail in case of invalid return value from fstat invocation.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(
            ::testing::Invoke([](const auto& /*handle*/, auto& stat_buffer) -> score::cpp::expected_blank<score::os::Error> {
                stat_buffer.st_size = -1L;
                return score::cpp::expected_blank<score::os::Error>{};
            }));

    //  We expect mmap not to be called irregardless of arguments
    EXPECT_CALL(*mman_mock_, mmap(_, _, _, _, _, _)).Times(0);

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_FALSE(result);
}

TEST_F(ReaderFactoryFixture, FstatReturningSizeTooSmallShallResultInEmptyOptional)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Reader creation shall fail in case of returning too small size from fstat invocation.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(
            ::testing::Invoke([](const auto& /*handle*/, auto& stat_buffer) -> score::cpp::expected_blank<score::os::Error> {
                static_assert(sizeof(SharedData) > 0UL, "SharedData cannot be zero-sized type");
                stat_buffer.st_size = sizeof(SharedData) - 1L;
                return score::cpp::expected_blank<score::os::Error>{};
            }));

    //  We expect mmap not to be called irregardless of arguments
    EXPECT_CALL(*mman_mock_, mmap(_, _, _, _, _, _)).Times(0);

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_EQ(result, nullptr);
}

TEST_F(ReaderFactoryFixture, MmapFailingShallResultInEmptyOptional)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Reader creation shall fail in case of failing invocation of mmap.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(
            ::testing::Invoke([](const auto& /*handle*/, auto& stat_buffer) -> score::cpp::expected_blank<score::os::Error> {
                stat_buffer.st_size = kSharedSize;
                return score::cpp::expected_blank<score::os::Error>{};
            }));

    EXPECT_CALL(*mman_mock_,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead,
                     score::os::Mman::Map::kShared,
                     kFileHandle,
                     kMmapOffset))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_EQ(result, nullptr);
}

TEST_F(ReaderFactoryFixture, SharedDataMemberPointingOutOfBoundsShallResultInEmptyOptional)
{
    RecordProperty("ParentRequirement", "SCR-861827");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Reader creation shall fail in case the shared data member is pointing out of bounds.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(
            ::testing::Invoke([](const auto& /*handle*/, auto& stat_buffer) -> score::cpp::expected_blank<score::os::Error> {
                stat_buffer.st_size = kSharedSize;
                return score::cpp::expected_blank<score::os::Error>{};
            }));

    EXPECT_CALL(*mman_mock_,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead,
                     score::os::Mman::Map::kShared,
                     kFileHandle,
                     kMmapOffset))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{&buffer_}));

    shared_data_.linear_buffer_1_offset = kSharedSize + 1UL;

    EXPECT_CALL(*mman_mock_, munmap(_, kSharedSize)).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_EQ(result, nullptr);
}

TEST_F(ReaderFactoryFixture, UnexpectedPidShallResultInEmptyOptional)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Reader creation shall fail in case of using unexpected pid.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(
            ::testing::Invoke([](const auto& /*handle*/, auto& stat_buffer) -> score::cpp::expected_blank<score::os::Error> {
                stat_buffer.st_size = kSharedSize;
                return score::cpp::expected_blank<score::os::Error>{};
            }));

    EXPECT_CALL(*mman_mock_,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead,
                     score::os::Mman::Map::kShared,
                     kFileHandle,
                     kMmapOffset))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{&buffer_}));

    shared_data_.producer_pid = 0x1;

    EXPECT_CALL(*mman_mock_, munmap(_, kSharedSize)).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_EQ(result, nullptr);
}

TEST_F(ReaderFactoryFixture, ProperSetupShallResultValidReader)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Testing the creation of the reader correctly.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(
            ::testing::Invoke([](const auto& /*handle*/, auto& stat_buffer) -> score::cpp::expected_blank<score::os::Error> {
                stat_buffer.st_size = kSharedSize;
                return score::cpp::expected_blank<score::os::Error>{};
            }));

    EXPECT_CALL(*mman_mock_,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead,
                     score::os::Mman::Map::kShared,
                     kFileHandle,
                     kMmapOffset))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{&buffer_}));

    //  Memory shall not be unmapped until Reader is destructed
    EXPECT_CALL(*mman_mock_, munmap(_, kSharedSize)).Times(0);

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_NE(result, nullptr);

    EXPECT_CALL(*mman_mock_, munmap(_, kSharedSize)).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));
}

TEST_F(ReaderFactoryFixture, UnmapFailureShallResultValidReader)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies returning a vaild reader in case of unmap failure.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    EXPECT_CALL(*stat_mock_, fstat(kFileHandle, _))
        .WillOnce(
            ::testing::Invoke([](const auto& /*handle*/, auto& stat_buffer) -> score::cpp::expected_blank<score::os::Error> {
                stat_buffer.st_size = kSharedSize;
                return score::cpp::expected_blank<score::os::Error>{};
            }));

    EXPECT_CALL(*mman_mock_,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead,
                     score::os::Mman::Map::kShared,
                     kFileHandle,
                     kMmapOffset))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{&buffer_}));

    auto result = factory_.Create(kFileHandle, kExpectedPid);
    EXPECT_NE(result, nullptr);

    EXPECT_CALL(*mman_mock_, munmap(_, kSharedSize))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
}

TEST(ReaderFactoryTests, DefaultShallCreateReaderFactoryImpl)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies that the default reader factory shall create a reader factory implementation instance.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    auto result = ReaderFactory::Default(score::cpp::pmr::get_default_resource());
    ASSERT_NE(result, nullptr);
    ASSERT_NO_THROW(ASSERT_NE(dynamic_cast<ReaderFactoryImpl*>(result.get()), nullptr));
}

TEST(ReaderFactoryTests, NullPtrResourceShallNotCreateReaderFactoryImpl)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies that the null ptr resource shall not create a reader factory implementation instance.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    auto result = ReaderFactory::Default(nullptr);
    ASSERT_EQ(result, nullptr);
    ASSERT_EQ(dynamic_cast<ReaderFactoryImpl*>(result.get()), nullptr);
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
