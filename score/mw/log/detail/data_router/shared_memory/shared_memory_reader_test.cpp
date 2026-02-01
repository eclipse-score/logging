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

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_writer.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <array>

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
using ::testing::MockFunction;

constexpr auto kRingSize = 4UL * 1024UL;
constexpr std::array<char, 10UL> kTestDataSample{"test data"};

class SharedMemoryReaderFixture : public ::testing::Test
{
  public:
    SharedMemoryReaderFixture()
        : shared_data{},
          shared_memory_reader(
              shared_data,
              AlternatingReadOnlyReader{shared_data.control_block,
                                        score::cpp::span<Byte>(stack_based_shared_memory[0].data(), kRingSize / 2UL),
                                        score::cpp::span<Byte>(stack_based_shared_memory[1].data(), kRingSize / 2UL)},
              UnmapCallback{}),
          shared_memory_writer(InitializeSharedData(shared_data), UnmapCallback{})
    {
        stack_based_shared_memory[0].fill(0x00);
        stack_based_shared_memory[1].fill(0x00);

        shared_data.linear_buffer_1_offset = sizeof(SharedData);
        shared_data.linear_buffer_2_offset = sizeof(SharedData) + kRingSize / 2UL;

        shared_data.control_block.control_block_even.data =
            score::cpp::span<Byte>(stack_based_shared_memory[0].data(), kRingSize / 2UL);
        shared_data.control_block.control_block_odd.data =
            score::cpp::span<Byte>(stack_based_shared_memory[1].data(), kRingSize / 2UL);
    }

    SharedData shared_data;
    SharedMemoryReader shared_memory_reader;
    SharedMemoryWriter shared_memory_writer;
    std::array<char, kRingSize> stack_based_shared_memory[2];
};

class TypeInfoTest
{
  public:
    void Copy(const score::cpp::span<Byte> data) const
    {
        std::memcpy(data.data(), type_.data(), type_.size());
    }
    std::size_t size() const
    {
        return sizeof(type_);
    }

  private:
    const std::array<char, 10UL> type_{"test::int"};
};

TEST_F(SharedMemoryReaderFixture, GetterShallReadSharedDataNumberOfDropsInvalidSize)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of getting the number of drops invalid size value properly.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static constexpr Length kNumberOfDrops{13UL};
    shared_data.number_of_drops_invalid_size.store(kNumberOfDrops);

    EXPECT_EQ(kNumberOfDrops, shared_memory_reader.GetNumberOfDropsWithInvalidSize());
}

TEST_F(SharedMemoryReaderFixture, GetterShallReadSharedDataNumberOfDropsBufferFull)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of getting the number of drops buffer full value properly.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static constexpr Length kNumberOfDrops{17UL};
    shared_data.number_of_drops_buffer_full.store(kNumberOfDrops);

    EXPECT_EQ(kNumberOfDrops, shared_memory_reader.GetNumberOfDropsWithBufferFull());
}

TEST_F(SharedMemoryReaderFixture, GetterShallReadSharedDataSizeOfDropsBufferFull)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of getting the size of drops buffer full value properly.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static constexpr Length kSizeOfDrops{2048UL};
    shared_data.size_of_drops_buffer_full.store(kSizeOfDrops);

    EXPECT_EQ(kSizeOfDrops, shared_memory_reader.GetSizeOfDropsWithBufferFull());
}

TEST_F(SharedMemoryReaderFixture, RingBufferSizeShallReturnValueBasedOnControlBlock)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of getting ring buffer size bytes value properly.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_EQ(kRingSize, shared_memory_reader.GetRingBufferSizeBytes());
}

TEST_F(SharedMemoryReaderFixture, UnmapCallbackShallBeCalledWhenDestructing)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "Verifies that the Unmap callback will get called when destructing the shared memory reader instance.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    MockFunction<void()> check;
    UnmapCallback callback = [&check]() noexcept {
        check.Call();
    };

    SharedMemoryReader reader(
        shared_data,
        AlternatingReadOnlyReader{shared_data.control_block,
                                  score::cpp::span<Byte>(stack_based_shared_memory[0].data(), kRingSize / 2UL),
                                  score::cpp::span<Byte>(stack_based_shared_memory[1].data(), kRingSize / 2UL)},
        std::move(callback));

    EXPECT_CALL(check, Call()).Times(1);
}

TEST_F(SharedMemoryReaderFixture, UnmapCallbackShallNotBeCalledWhenMoving)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies that the Unmap callback will not get called in case of moving the shared memory reader to "
                   "another instance.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    MockFunction<void()> check;
    UnmapCallback callback = [&check]() noexcept {
        check.Call();
    };

    SharedMemoryReader reader(
        shared_data,
        AlternatingReadOnlyReader{shared_data.control_block,
                                  score::cpp::span<Byte>(stack_based_shared_memory[0].data(), kRingSize / 2UL),
                                  score::cpp::span<Byte>(stack_based_shared_memory[1].data(), kRingSize / 2UL)},
        std::move(callback));

    EXPECT_CALL(check, Call()).Times(0);

    SharedMemoryReader shared_memory_reader_created_by_move(std::move(reader));

    EXPECT_CALL(check, Call()).Times(1);
}

TEST_F(SharedMemoryReaderFixture, ReaderNotNotifiedShallNotPerformReads)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the in-ability of doing read in case of missing Notifying acquisition first.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer.TryRegisterType(TypeInfoTest{});
    EXPECT_TRUE(type_id.has_value());

    shared_memory_writer.AllocAndWrite(
        [](auto span) noexcept {
            EXPECT_EQ(span.size(), kTestDataSample.size());
            std::memcpy(span.data(), kTestDataSample.data(), kTestDataSample.size());
        },
        type_id.value(),
        kTestDataSample.size());

    const auto read_acquire_result = shared_memory_writer.ReadAcquire();

    //  Skip notification:
    std::ignore = read_acquire_result;
    //  shared_memory_reader_.NotifyAcquisition(read_acquire_result);

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };
    auto on_new_record = [&](const score::mw::log::detail::SharedMemoryRecord&) noexcept {
        FAIL();
    };

    //  Read without first notifying reader about data acquisiton
    shared_memory_reader.Read(on_new_type, on_new_record);
}

TEST_F(SharedMemoryReaderFixture, ReadDetached)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the in-ability of doing read in case of missing Notifying acquisition first.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer.TryRegisterType(TypeInfoTest{});
    EXPECT_TRUE(type_id.has_value());

    shared_memory_writer.AllocAndWrite(
        [](auto span) noexcept {
            EXPECT_EQ(span.size(), kTestDataSample.size());
            std::memcpy(span.data(), kTestDataSample.data(), kTestDataSample.size());
        },
        type_id.value(),
        kTestDataSample.size());

    shared_memory_writer.ReadAcquire();

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration&) noexcept {};
    auto on_new_record = [&](const score::mw::log::detail::SharedMemoryRecord&) noexcept {};

    // prepare reader to be able to read data before writer detached
    shared_memory_reader.NotifyAcquisitionSetReader(ReadAcquireResult{0});

    // detach writer and read
    shared_data.writer_detached = true;
    EXPECT_TRUE(shared_memory_reader.Read(on_new_type, on_new_record).has_value());
}

TEST_F(SharedMemoryReaderFixture, ReaderNotifiedShallReturnNulloptWhenNotReleasedByWriters)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the in-ability of doing read in case of missing Notifying acquisition first.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer.TryRegisterType(TypeInfoTest{});
    EXPECT_TRUE(type_id.has_value());

    ReadAcquireResult read_acquire_result{};
    shared_memory_writer.AllocAndWrite(
        [this, &read_acquire_result](auto span) noexcept {
            EXPECT_EQ(span.size(), kTestDataSample.size());

            read_acquire_result = shared_memory_writer.ReadAcquire();
            //  Because we are inside the buffer access callback, buffer is not released, thus it is expected that a try
            //  to acquire the buffer is not successful:
            EXPECT_EQ(shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_result), std::nullopt);
        },
        type_id.value(),
        kTestDataSample.size());

    //  Now that all writers released buffer, expect notification to have a value:
    auto result_length = shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_result);
    EXPECT_TRUE(result_length.has_value());
}

TEST_F(SharedMemoryReaderFixture, ReaderNotifiedShallAcquireData)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of doing read when Notifying acquisition first.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    auto on_new_type_discard = [&](const score::mw::log::detail::TypeRegistration&) noexcept {};
    auto on_new_record_discard = [&](const score::mw::log::detail::SharedMemoryRecord&) noexcept {};
    EXPECT_FALSE(shared_memory_reader.Read(on_new_type_discard, on_new_record_discard).has_value());

    const auto type_id = shared_memory_writer.TryRegisterType(TypeInfoTest{});

    EXPECT_FALSE(shared_memory_reader.Read(on_new_type_discard, on_new_record_discard).has_value());

    shared_memory_writer.AllocAndWrite(
        [](auto span) noexcept {
            EXPECT_EQ(span.size(), kTestDataSample.size());
            std::memcpy(span.data(), kTestDataSample.data(), kTestDataSample.size());
        },
        type_id.value(),
        kTestDataSample.size());

    EXPECT_FALSE(shared_memory_reader.Read(on_new_type_discard, on_new_record_discard).has_value());

    //  trigger the buffer switch:
    const auto read_acquire_result = shared_memory_writer.ReadAcquire();

    //  Expect acquired buffer to have non-zero acquired bytes:
    const auto reading_buffer_peek_result =
        shared_memory_reader.PeekNumberOfBytesAcquiredInBuffer(read_acquire_result.acquired_buffer);
    EXPECT_TRUE(reading_buffer_peek_result.has_value());
    EXPECT_GT(reading_buffer_peek_result.value(), 0UL);

    //  Expect next buffer to have zero acquired bytes:
    const auto writing_buffer_peek_result =
        shared_memory_reader.PeekNumberOfBytesAcquiredInBuffer(read_acquire_result.acquired_buffer + 1);
    EXPECT_TRUE(writing_buffer_peek_result.has_value());
    EXPECT_EQ(writing_buffer_peek_result.value(), 0UL);

    //  Wait on a block to be released by writers:
    while (not shared_memory_reader.IsBlockReleasedByWriters(read_acquire_result.acquired_buffer))
    {
    }
    const auto acquired_length = shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_result);
    EXPECT_TRUE(acquired_length.has_value());
    EXPECT_GT(acquired_length.value(), kTestDataSample.size());

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };
    auto on_new_record = [&](const score::mw::log::detail::SharedMemoryRecord& record) noexcept {
        EXPECT_EQ(record.header.type_identifier, type_id.value());
    };

    const auto acquired_result = shared_memory_reader.Read(on_new_type, on_new_record);
    ASSERT_TRUE(acquired_result.has_value());
    EXPECT_LT(kTestDataSample.size() + TypeInfoTest{}.size(), acquired_result.value());

    //  Expect that after reader is depleated it should not return any data:
    EXPECT_FALSE(shared_memory_reader.Read(on_new_type_discard, on_new_record_discard).has_value());
}

TEST_F(SharedMemoryReaderFixture, WriterAcquireShallAllowDataToBeWritten)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of writing data when allocate for writing.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer.TryRegisterType(TypeInfoTest{});

    MockFunction<void()> check;
    UnmapCallback callback = [&check]() noexcept {
        check.Call();
    };

    auto write = [&, this]() noexcept {
        shared_memory_writer.AllocAndWrite(
            [&](auto span) noexcept {
                check.Call();
                EXPECT_EQ(span.size(), kTestDataSample.size());
                std::memcpy(span.data(), kTestDataSample.data(), kTestDataSample.size());
            },
            type_id.value(),
            kTestDataSample.size());
    };

    EXPECT_CALL(check, Call()).Times(1);

    write();

    //  Performs Switch in alternating buffer
    const auto read_acquire_result = shared_memory_writer.ReadAcquire();

    EXPECT_CALL(check, Call()).Times(1);

    write();

    shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_result);

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };

    MockFunction<void(const SharedMemoryRecord&)> check_record;
    auto on_new_record = [&](const SharedMemoryRecord& record) noexcept {
        check_record.Call(record);
        EXPECT_EQ(record.header.type_identifier, type_id.value());
    };

    EXPECT_CALL(check_record, Call(_)).Times(1);

    //  Read without first notifying reader about data acquisiton
    shared_memory_reader.Read(on_new_type, on_new_record);

    //  Performs Switch in alternating buffer
    const auto read_acquire_second_result = shared_memory_writer.ReadAcquire();

    shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_second_result);

    EXPECT_CALL(check_record, Call(_)).Times(1);

    shared_memory_reader.Read(on_new_type, on_new_record);
}

TEST_F(SharedMemoryReaderFixture, CorruptedHeaderSizeTooSmallShallIgnoreEntry)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies ignoring the entry in case of using corrupted header.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer.TryRegisterType(TypeInfoTest{});

    MockFunction<void()> check;
    UnmapCallback callback = [&check]() noexcept {
        check.Call();
    };

    EXPECT_CALL(check, Call()).Times(1);

    shared_memory_writer.AllocAndWrite(
        [&](auto span) noexcept {
            check.Call();
            EXPECT_EQ(span.size(), 0UL);
            //  Hack a way to internal Length field and decrease it relying on header located right before payload.
            //  It is forbidden to use similar way in production code!
            constexpr auto kHeaderSize = sizeof(BufferEntryHeader);
            Length overwrite_length_value_hack = kHeaderSize - 1L;

            std::memcpy(span.data() - sizeof(Length) - kHeaderSize, &overwrite_length_value_hack, sizeof(Length));
        },
        type_id.value(),
        0UL);

    //  Performs Switch in alternating buffer
    const auto read_acquire_result = shared_memory_writer.ReadAcquire();

    shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_result);

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };

    MockFunction<void(const SharedMemoryRecord&)> check_record;
    auto on_new_record = [&](const SharedMemoryRecord& record) noexcept {
        check_record.Call(record);
    };

    //  Do not expect call to broken entry
    EXPECT_CALL(check_record, Call(_)).Times(0);

    shared_memory_reader.Read(on_new_type, on_new_record);
}

TEST_F(SharedMemoryReaderFixture, CorruptedRegistrationHeaderSizeTooSmallShallIgnoreEntry)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies ignoring the entry in case of using corrupted registration header.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    class CorruptHeaderTypeInfoTest
    {
      public:
        void Copy(const score::cpp::span<Byte> data) const
        {
            std::memcpy(data.data(), type_.data(), type_.size());
            constexpr auto kHeaderSize = sizeof(BufferEntryHeader);
            Length overwrite_length_value_hack = kHeaderSize;
            std::memcpy(data.data() - sizeof(Length) - kHeaderSize - sizeof(TypeIdentifier),
                        &overwrite_length_value_hack,
                        sizeof(Length));
        }
        std::size_t size() const
        {
            return sizeof(type_);
        }

      private:
        const std::array<char, 10UL> type_{"test::int"};
    };

    const auto type_id = shared_memory_writer.TryRegisterType(CorruptHeaderTypeInfoTest{});

    MockFunction<void()> check;
    UnmapCallback callback = [&check]() noexcept {
        check.Call();
    };

    EXPECT_CALL(check, Call()).Times(1);

    shared_memory_writer.AllocAndWrite(
        [&](auto) noexcept {
            check.Call();
        },
        type_id.value(),
        0UL);

    //  Performs Switch in alternating buffer
    const auto read_acquire_result = shared_memory_writer.ReadAcquire();

    shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_result);

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };

    MockFunction<void(const SharedMemoryRecord&)> check_record;
    auto on_new_record = [&](const SharedMemoryRecord& record) noexcept {
        check_record.Call(record);
    };

    //  Do not expect call to broken entry
    EXPECT_CALL(check_record, Call(_)).Times(0);

    shared_memory_reader.Read(on_new_type, on_new_record);
}

TEST_F(SharedMemoryReaderFixture, WriterDetachedShallAllowDataToBeReadWithoutSwitching)
{
    RecordProperty("ParentRequirement", "SCR-861827, SCR-12206795");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the ability of read the data without switching in case of detached the writer.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer.TryRegisterType(TypeInfoTest{});

    MockFunction<void()> check;
    UnmapCallback callback = [&check]() noexcept {
        check.Call();
    };

    auto write = [&, this]() noexcept {
        shared_memory_writer.AllocAndWrite(
            [&](auto span) noexcept {
                check.Call();
                EXPECT_EQ(span.size(), kTestDataSample.size());
                std::memcpy(span.data(), kTestDataSample.data(), kTestDataSample.size());
            },
            type_id.value(),
            kTestDataSample.size());
    };

    EXPECT_CALL(check, Call()).Times(1);

    write();

    //  Performs Switch in alternating buffer
    const auto read_acquire_result = shared_memory_writer.ReadAcquire();

    EXPECT_CALL(check, Call()).Times(1);

    write();

    shared_memory_reader.NotifyAcquisitionSetReader(read_acquire_result);

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };

    MockFunction<void(const SharedMemoryRecord&)> check_record;
    auto on_new_record = [&](const SharedMemoryRecord& record) noexcept {
        check_record.Call(record);
        EXPECT_EQ(record.header.type_identifier, type_id.value());
    };

    EXPECT_CALL(check_record, Call(_)).Times(2);

    //  Read without first notifying reader about data acquisiton
    shared_memory_reader.Read(on_new_type, on_new_record);

    shared_memory_reader.ReadDetached(on_new_type, on_new_record);

    //  After detaching writer no more calls are expected
    EXPECT_CALL(check_record, Call(_)).Times(0);
    shared_memory_reader.Read(on_new_type, on_new_record);
}
}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
