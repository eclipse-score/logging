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

#include "score/mw/log/detail/data_router/shared_memory/writer_factory.h"

#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/mman_mock.h"
#include "score/os/mocklib/stat_mock.h"
#include "score/os/mocklib/stdlib_mock.h"
#include "score/os/mocklib/unistdmock.h"

#include <sys/types.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

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
using ::testing::StrEq;

constexpr pid_t kPid{0x314};
constexpr std::int32_t kArbitratyUid = 12365432UL;
constexpr bool kDynamicFalse = false;
constexpr bool kDynamicTrue = true;
constexpr auto kDefaultRingSize = 1024UL;
constexpr auto kSharedSize = kDefaultRingSize + sizeof(SharedData);
constexpr auto kOverflowSize = std::numeric_limits<size_t>::max();
constexpr std::int32_t kFileDescriptor = 0x1;
constexpr auto kOpenReadFlags =
    score::os::Fcntl::Open::kCreate | score::os::Fcntl::Open::kReadWrite | score::os::Fcntl::Open::kExclusive;
constexpr auto kOpenReadFlagsDynamic =
    score::os::Fcntl::Open::kReadWrite | score::os::Fcntl::Open::kExclusive | score::os::Fcntl::Open::kCloseOnExec;
constexpr auto kOpenModeFlags =
    score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kReadGroup | score::os::Stat::Mode::kReadOthers;
constexpr auto kAlignRequirement = std::alignment_of<SharedData>::value;
const char kFileNameDynamic[] = "/tmp/logging-XXXXXX.shmem";

const std::string& GetSharedMemoryFileName()
{
    std::stringstream ss;
    ss << "/tmp/logging.UTST." << kArbitratyUid << ".shmem";
    ss.str();
    static const std::string kFileName{ss.str()};
    return kFileName;
}

class WriterFactoryFixture : public ::testing::Test
{
  public:
    WriterFactoryFixture() : shared_memory_file_name{GetSharedMemoryFileName()}

    {
        auto* memory_resource = score::cpp::pmr::get_default_resource();
        auto fcntl_mock = score::cpp::pmr::make_unique<score::os::FcntlMock>(memory_resource);
        auto unistd_mock = score::cpp::pmr::make_unique<score::os::UnistdMock>(memory_resource);
        auto mman_mock = score::cpp::pmr::make_unique<score::os::MmanMock>(memory_resource);
        auto stat_mock = score::cpp::pmr::make_unique<score::os::StatMock>(memory_resource);
        auto stdlib_mock = score::cpp::pmr::make_unique<score::os::StdlibMock>(memory_resource);

        fcntl_mock_raw_ptr = fcntl_mock.get();
        mman_mock_raw_ptr = mman_mock.get();
        unistd_mock_raw_ptr = unistd_mock.get();
        stat_mock_raw_ptr = stat_mock.get();
        stdlib_mock_raw_ptr = stdlib_mock.get();

        osal.fcntl_osal = std::move(fcntl_mock);
        osal.unistd = std::move(unistd_mock);
        osal.mman = std::move(mman_mock);
        osal.stdlib = std::move(stdlib_mock);
        osal.stat_osal = std::move(stat_mock);

        ON_CALL(*unistd_mock_raw_ptr, getuid()).WillByDefault(Return(kArbitratyUid));

        non_zero_address = &buffer[0];
        map_address = &buffer[0];
    }
    ~WriterFactoryFixture()
    {
        non_zero_address = nullptr;
        map_address = nullptr;
    }

    score::os::FcntlMock* fcntl_mock_raw_ptr;
    score::os::UnistdMock* unistd_mock_raw_ptr;
    score::os::StatMock* stat_mock_raw_ptr;
    score::os::StdlibMock* stdlib_mock_raw_ptr;
    score::os::MmanMock* mman_mock_raw_ptr;
    WriterFactory::OsalInstances osal;

    //  Mmap helpers:
    Byte buffer[kSharedSize + kAlignRequirement];
    void* non_zero_address;
    void* map_address;
    std::string shared_memory_file_name;
};

TEST(WriterFactory, MissingOsalShallResultInEmptyOptional)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the in-ability of creating shared memory writer in case of missing all osal parameters.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory::OsalInstances osal{nullptr, nullptr, nullptr};
    WriterFactory writer(std::move(osal));
    auto result = writer.Create(kDefaultRingSize, kDynamicFalse, "UTST");

    EXPECT_FALSE(result.has_value());
}

TEST(WriterFactory, MissingOsalUnistdAndOsalMmanShallResultInEmptyOptional)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verify that create writers returns empty in case of missing OSAL parameters for unistd and mman");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory::OsalInstances osal{
        score::cpp::pmr::make_unique<score::os::FcntlMock>(score::cpp::pmr::get_default_resource()), nullptr, nullptr};
    WriterFactory writer(std::move(osal));
    auto result = writer.Create(kDefaultRingSize, kDynamicFalse, "UTST");

    EXPECT_FALSE(result.has_value());
}

TEST(WriterFactory, MissingOsalMmanShallResultInEmptyOptional)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify that create writer returns empty in case of missing OSAL parameter for mman");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory::OsalInstances osal{score::cpp::pmr::make_unique<score::os::FcntlMock>(score::cpp::pmr::get_default_resource()),
                                      score::cpp::pmr::make_unique<score::os::UnistdMock>(score::cpp::pmr::get_default_resource()),
                                      nullptr};
    WriterFactory writer(std::move(osal));
    auto result = writer.Create(kDefaultRingSize, kDynamicFalse, "UTST");

    EXPECT_FALSE(result.has_value());
}

TEST(WriterFactory, MissingStatOsalShallResultInEmptyOptional)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify that create writer returns empty in case of missing OSAL parameter for mman");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory::OsalInstances osal{score::cpp::pmr::make_unique<score::os::FcntlMock>(score::cpp::pmr::get_default_resource()),
                                      score::cpp::pmr::make_unique<score::os::UnistdMock>(score::cpp::pmr::get_default_resource()),
                                      score::cpp::pmr::make_unique<score::os::MmanMock>(score::cpp::pmr::get_default_resource()),
                                      nullptr};
    WriterFactory writer(std::move(osal));
    auto result = writer.Create(kDefaultRingSize, kDynamicFalse, "UTST");

    EXPECT_FALSE(result.has_value());
}

TEST(WriterFactory, MissingStdlibShallResultInEmptyOptional)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verify that create writer returns empty in case of missing OSAL parameter for mman");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory::OsalInstances osal{score::cpp::pmr::make_unique<score::os::FcntlMock>(score::cpp::pmr::get_default_resource()),
                                      score::cpp::pmr::make_unique<score::os::UnistdMock>(score::cpp::pmr::get_default_resource()),
                                      score::cpp::pmr::make_unique<score::os::MmanMock>(score::cpp::pmr::get_default_resource()),
                                      score::cpp::pmr::make_unique<score::os::StatMock>(score::cpp::pmr::get_default_resource()),
                                      nullptr};
    WriterFactory writer(std::move(osal));
    auto result = writer.Create(kDefaultRingSize, kDynamicFalse, "UTST");

    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, WhenTheFileExistsItShallBeUnlinked)
{
    RecordProperty("ParentRequirement", "SCR-1016729, SCR-26319707");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the ability of unlink the exist file. The component shall set the FD_CLOEXEC (or "
                   "O_CLOEXEC) flag on all the file descriptor it owns");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*unistd_mock_raw_ptr, access(StrEq(shared_memory_file_name), score::os::Unistd::AccessMode::kExists))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_CALL(*unistd_mock_raw_ptr, unlink(StrEq(shared_memory_file_name)))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_CALL(
        *fcntl_mock_raw_ptr,
        open(StrEq(shared_memory_file_name), kOpenReadFlags | score::os::Fcntl::Open::kCloseOnExec, kOpenModeFlags))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    //  Expect to exit before a call to next mocking feature
    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(_, _)).Times(0);

    auto result = writer.Create(kDefaultRingSize, kDynamicFalse, "UTST");
    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, WhenTheFileExistsAndCannotBeUnlinkedItShallContinueWithExecution)
{
    RecordProperty("ParentRequirement", "SCR-1016729, SCR-26319707");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the ability of continue the execution in case of in-ability of unlink an exist file. The "
                   "component shall set the FD_CLOEXEC (or O_CLOEXEC) flag on all the file descriptor it owns");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*unistd_mock_raw_ptr, access(StrEq(shared_memory_file_name), score::os::Unistd::AccessMode::kExists))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_CALL(*unistd_mock_raw_ptr, unlink(StrEq(shared_memory_file_name)))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    EXPECT_CALL(
        *fcntl_mock_raw_ptr,
        open(StrEq(shared_memory_file_name), kOpenReadFlags | score::os::Fcntl::Open::kCloseOnExec, kOpenModeFlags))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    //  Expect to exit before a call to next mocking feature
    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(_, _)).Times(0);

    auto result = writer.Create(kDefaultRingSize, kDynamicFalse, "UTST");
    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, InDynamicFileExistanceShallNotBeChecked)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies that we should not check a dynamic file creation.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*unistd_mock_raw_ptr, access(_, _)).Times(0);

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, FailureToOpenFileShallResultInEmptyOptionalResult)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of creating the writer in case of failing to open a file.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    //  Expect to exit before a call to next mocking feature
    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(_, _)).Times(0);

    auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, FailureToTruncateFileShallResultInEmptyOptionalResult)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the in-ability of creating the writer in case of failing to truncate a file.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, kSharedSize))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    //  Expect unlink to be called before exit
    EXPECT_CALL(*unistd_mock_raw_ptr, unlink(StrEq(kFileNameDynamic)))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    //  We expect mmap not to be called irregardless of arguments
    EXPECT_CALL(*mman_mock_raw_ptr, mmap(_, _, _, _, _, _)).Times(0);

    auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, MakeSureThatOpenCallWillOnlyBeDoneWithCorrectOpenReadFlags)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Logging shared-memory files shall have read-only posix file permission for group and others");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));
    EXPECT_CALL(
        *fcntl_mock_raw_ptr,
        open(StrEq(kFileNameDynamic), score::os::Fcntl::Open::kCreate | score::os::Fcntl::Open::kReadWrite, kOpenModeFlags))
        .Times(0);
    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags)).Times(1);

    auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
}

TEST_F(WriterFactoryFixture, FailureToMapFileShallResultInEmptyOptionalResult)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the in-ability of creating the writer in case of failing to map a file.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, kSharedSize))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_CALL(*mman_mock_raw_ptr,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                     score::os::Mman::Map::kShared,
                     kFileDescriptor,
                     0))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));

    //  Expect unlink to be called before exit
    EXPECT_CALL(*unistd_mock_raw_ptr, unlink(StrEq(kFileNameDynamic)))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, WhenAllMocksReturnValidShallResultValidOptionalResult)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the ability of creating the writer in case of all osal parameters are valid.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, kSharedSize))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_CALL(*mman_mock_raw_ptr,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                     score::os::Mman::Map::kShared,
                     kFileDescriptor,
                     0))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{map_address}));

    EXPECT_CALL(*unistd_mock_raw_ptr, getpid()).WillOnce(Return(kPid));

    const auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_TRUE(result.has_value());

    EXPECT_CALL(*mman_mock_raw_ptr, munmap(_, kSharedSize)).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));
}

TEST_F(WriterFactoryFixture, WhenMmapIsValidAndUnmmapIsFailingItShallPrintCerrMessage)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Should fail in case of mmap succeeded and munmap failed.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, kSharedSize))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_CALL(*mman_mock_raw_ptr,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                     score::os::Mman::Map::kShared,
                     kFileDescriptor,
                     0))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{map_address}));

    EXPECT_CALL(*unistd_mock_raw_ptr, getpid()).WillOnce(Return(kPid));

    const auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_TRUE(result.has_value());

    EXPECT_CALL(*mman_mock_raw_ptr, munmap(_, kSharedSize))
        .WillOnce(Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL))));
}

TEST_F(WriterFactoryFixture, MmapReturnsNullptrValueShallCallUnmapAndReturnEmpty)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "Verifies that when Mmap calling returns nullptr it will lead invoking to munmap with returning empty.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, kSharedSize))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    EXPECT_CALL(*mman_mock_raw_ptr,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                     score::os::Mman::Map::kShared,
                     kFileDescriptor,
                     0))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{nullptr}));

    EXPECT_CALL(*unistd_mock_raw_ptr, getpid()).Times(0);

    EXPECT_CALL(*mman_mock_raw_ptr, munmap(_, kSharedSize)).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    const auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_FALSE(result.has_value());
}

Byte* GetImproperAlignment(void* map_address)
{
    if (0 == reinterpret_cast<uint64_t>(map_address) % 2)
    {
        return static_cast<Byte*>(map_address) + 1UL;
    }
    else
    {
        return static_cast<Byte*>(map_address);
    }
}

TEST_F(WriterFactoryFixture, MmapReturingImproperAlignedMemoryShallCallUnmapAndReturnEmpty)
{
    RecordProperty("ParentRequirement", "SCR-1016729");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies that when Mmap calling returns improper aligned memory it will lead invoking to munmap "
                   "with returning empty.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("Priority", "3");

    WriterFactory writer(std::move(osal));

    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, kSharedSize))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    Byte* improper_alignment_address = GetImproperAlignment(map_address);

    EXPECT_CALL(*mman_mock_raw_ptr,
                mmap(nullptr,
                     kSharedSize,
                     score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                     score::os::Mman::Map::kShared,
                     kFileDescriptor,
                     0))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{improper_alignment_address}));

    EXPECT_CALL(*unistd_mock_raw_ptr, getpid()).Times(0);

    EXPECT_CALL(*mman_mock_raw_ptr, munmap(_, kSharedSize)).WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    const auto result = writer.Create(kDefaultRingSize, kDynamicTrue, "UTST");
    EXPECT_FALSE(result.has_value());
}

TEST_F(WriterFactoryFixture, UnexpectedBufferSizeWithOverflowShallMakeCerrOutput)
{
    // Tests behavior when ring buffer size causes integer overflow during total size calculation.
    // Expected size is sizeof(SharedData) which contains
    const std::size_t expected_shared_data_size = 143;
    WriterFactory writer(std::move(osal));

    // Step 1: Open shared memory file succeeds
    EXPECT_CALL(*fcntl_mock_raw_ptr, open(StrEq(kFileNameDynamic), kOpenReadFlagsDynamic, kOpenModeFlags))
        .WillOnce(Return(score::cpp::expected<std::int32_t, score::os::Error>{kFileDescriptor}));

    // Step 2: Truncate file to expected size (overflow clamped to SharedData size)
    EXPECT_CALL(*unistd_mock_raw_ptr, ftruncate(kFileDescriptor, expected_shared_data_size))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    // Step 3: Memory mapping fails (returns nullptr)
    EXPECT_CALL(*mman_mock_raw_ptr,
                mmap(nullptr,
                     expected_shared_data_size,
                     score::os::Mman::Protection::kRead | score::os::Mman::Protection::kWrite,
                     score::os::Mman::Map::kShared,
                     kFileDescriptor,
                     0))
        .WillOnce(Return(score::cpp::expected<void*, score::os::Error>{nullptr}));

    // Step 4: getpid should not be called due to mmap failure
    EXPECT_CALL(*unistd_mock_raw_ptr, getpid()).Times(0);

    // Step 5: Cleanup - unmap is called during error handling
    EXPECT_CALL(*mman_mock_raw_ptr, munmap(_, expected_shared_data_size))
        .WillOnce(Return(score::cpp::expected_blank<score::os::Error>{}));

    // Verify: Create fails and returns empty optional
    const auto result = writer.Create(kOverflowSize, kDynamicTrue, "UTST");
    EXPECT_FALSE(result.has_value());
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
