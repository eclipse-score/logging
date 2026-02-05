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

#include "score/mw/log/detail/data_router/shared_memory/shared_memory_writer.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_reader.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <array>
#include <thread>

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

static constexpr std::array<char, 10UL> test_data_sample{"test data"};

static constexpr auto kNumberOfThreads = 10UL;
static constexpr auto kNumberOfActions = 5UL;
static constexpr auto kNumberOfTests = 50UL;

static constexpr auto kRingSize = 4UL * 1024UL;

class SharedMemoryWriterFixture : public ::testing::Test
{
  public:
    SharedMemoryWriterFixture()
        : shared_data_{}, shared_memory_writer_(InitializeSharedData(shared_data_), UnmapCallback{})
    {
        stack_based_shared_memory[0].fill(0x00);
        stack_based_shared_memory[1].fill(0x00);

        shared_data_.linear_buffer_1_offset = sizeof(SharedData);
        shared_data_.linear_buffer_2_offset = sizeof(SharedData) + kRingSize / 2UL;

        shared_data_.control_block.control_block_even.data =
            score::cpp::span<Byte>(&stack_based_shared_memory[0][0], kRingSize / 2UL);
        shared_data_.control_block.control_block_odd.data =
            score::cpp::span<Byte>(&stack_based_shared_memory[1][0], kRingSize / 2UL);

        AlternatingReadOnlyReader read_only_reader{
            shared_data_.control_block,
            shared_data_.control_block.control_block_even.data,
            shared_data_.control_block.control_block_odd.data,
        };
        shared_memory_reader_ =
            std::make_unique<SharedMemoryReader>(shared_data_, std::move(read_only_reader), UnmapCallback{});
    }

    SharedData shared_data_;
    std::unique_ptr<SharedMemoryReader> shared_memory_reader_;
    std::array<char, kRingSize> stack_based_shared_memory[2];

    SharedMemoryWriter shared_memory_writer_;
};

class SharedMemoryWriterOversizedRequestsFixture : public SharedMemoryWriterFixture,
                                                   public testing::WithParamInterface<uint32_t>
{
  public:
};

INSTANTIATE_TEST_SUITE_P(OversizedRequests,
                         SharedMemoryWriterOversizedRequestsFixture,
                         testing::Values(SharedMemoryWriter::GetMaxPayloadSize(),
                                         SharedMemoryWriter::GetMaxPayloadSize() + 1UL));

class TypeInfoTest
{
  public:
    void Copy(const score::cpp::span<Byte> data) const
    {
        std::memcpy(data.data(), type.data(), type.size());
    }
    std::size_t size() const
    {
        return sizeof(type);
    }

  private:
    const std::array<char, 10UL> type{"test::int"};
};

class TypeInfoTestOversized
{
  public:
    void Copy(const score::cpp::span<Byte>) const {}
    std::size_t size() const
    {
        constexpr std::size_t type_max = std::numeric_limits<score::mw::log::detail::TypeIdentifier>::max() + 1UL;
        static_assert(type_max > 0 && (std::numeric_limits<std::size_t>::max() > type_max),
                      "Type requirements not met");
        return type_max;
    }
};

TEST_F(SharedMemoryWriterFixture, OversizedTypeRegisterShallReturnEmpty)
{
    RecordProperty("Requirement", "SCR-1633921,SCR-861534,SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "If a type info returns an oversized length the type registration shall fail.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    const auto result = shared_memory_writer_.TryRegisterType(TypeInfoTestOversized{});

    EXPECT_FALSE(result.has_value());
}

TEST_F(SharedMemoryWriterFixture, BasicRegisterShallReturnValue)
{
    RecordProperty("Requirement", "SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "For a valid type the type registration shall succeed.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    const auto result = shared_memory_writer_.TryRegisterType(TypeInfoTest{});

    EXPECT_TRUE(result.has_value());
}

TEST_F(SharedMemoryWriterFixture, RegistrationInSequenceShallYieldUniqueTypes)
{
    RecordProperty("Requirement", "SCR-1633921");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "For two valid types the type registration shall return unique identifiers.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto result = shared_memory_writer_.TryRegisterType(TypeInfoTest{});
    const auto second_result = shared_memory_writer_.TryRegisterType(TypeInfoTest{});

    EXPECT_NE(result, second_result);
}

TEST_F(SharedMemoryWriterFixture, ShallHandleOverflowAndNotFail)
{
    RecordProperty("Requirement", "SCR-1633921,SCR-861534,SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "If more types than supported are registered, the registration shall fail.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto first = shared_memory_writer_.TryRegisterType(TypeInfoTest{});
    for (auto i = 0UL; i < kRingSize; i++)  //  much more then it is possible
    {
        const auto result = shared_memory_writer_.TryRegisterType(TypeInfoTest{});
    }
    const auto result = shared_memory_writer_.TryRegisterType(TypeInfoTest{});
    EXPECT_FALSE(result.has_value());
}

TEST_F(SharedMemoryWriterFixture, SingleWriteReadShallBeReadPresentingTheSameValues)
{
    RecordProperty("Requirement", "SCR-1633921,SCR-861534,SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Reading shall return the same type and message as written.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer_.TryRegisterType(TypeInfoTest{});

    Byte* get_buffer_space_address = nullptr;
    shared_memory_writer_.AllocAndWrite(
        [&get_buffer_space_address](auto span) {
            EXPECT_EQ(span.size(), test_data_sample.size());
            std::memcpy(span.data(), test_data_sample.data(), test_data_sample.size());
            get_buffer_space_address = span.data();
        },
        type_id.value(),
        test_data_sample.size());

    const auto read_acquire_result = shared_memory_writer_.ReadAcquire();

    //  Datarouter part after acquisition
    shared_memory_reader_->NotifyAcquisitionSetReader(read_acquire_result);

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };
    auto on_new_record = [&](const score::mw::log::detail::SharedMemoryRecord& record) noexcept {
        EXPECT_EQ(record.header.type_identifier, type_id.value());
        EXPECT_EQ(record.payload.data(), get_buffer_space_address);
    };

    shared_memory_reader_->Read(on_new_type, on_new_record);
}

TEST_P(SharedMemoryWriterOversizedRequestsFixture, WriteWithTooBigRequestShallBeRejected)
{
    RecordProperty("Requirement", "SCR-861534,SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Out of bounds data shall be dropped.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer_.TryRegisterType(TypeInfoTest{});
    const auto oversized = GetParam();

    //  Expect that there is no data to be written
    shared_memory_writer_.AllocAndWrite(
        [&](auto) {
            FAIL();
        },
        type_id.value(),
        oversized);

    const auto read_acquire_result = shared_memory_writer_.ReadAcquire();

    //  Datarouter part after acquisition
    shared_memory_reader_->NotifyAcquisitionSetReader(read_acquire_result);

    auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
        EXPECT_EQ(registration.type_id, type_id.value());
    };

    auto on_new_record = [&](const score::mw::log::detail::SharedMemoryRecord&) noexcept {
        FAIL();
    };

    shared_memory_reader_->Read(on_new_type, on_new_record);
}

TEST_F(SharedMemoryWriterFixture, MultipleConcurentRegistrationShallBeValidInCount)
{
    RecordProperty("Requirement", "SCR-861534,SCR-1016719,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Ensures that multiple concurrent type registrations shall be performed without causing memory "
                   "corruption or race conditions.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    // When writing into it from multiple threads
    std::array<std::thread, kNumberOfThreads> threads{};
    for (std::uint8_t counter{}; counter < threads.size(); counter++)
    {
        threads.at(counter) = std::thread([this]() noexcept {
            for (auto number = 0UL; number < kNumberOfActions; number++)
            {
                const auto type_id = shared_memory_writer_.TryRegisterType(TypeInfoTest{});
            }
        });
    }

    // Then no memory corruption or race conditions happen (checked by TSAN, ASAN, valgrind)
    for (auto& thread : threads)
    {
        thread.join();
    }

    const auto read_acquire_result = shared_memory_writer_.ReadAcquire();

    //  Datarouter part after acquisition
    shared_memory_reader_->NotifyAcquisitionSetReader(read_acquire_result);

    auto count = 0UL;
    auto on_new_type = [&count](const score::mw::log::detail::TypeRegistration&) noexcept {
        count++;
    };
    auto on_new_record = [&](const score::mw::log::detail::SharedMemoryRecord&) noexcept {
        FAIL();
    };

    shared_memory_reader_->Read(on_new_type, on_new_record);

    EXPECT_EQ(count, kNumberOfThreads * kNumberOfActions);
}

TEST_F(SharedMemoryWriterFixture, MultipleConcurrentWritesShallAllBeReceivedValidInCountAndValue)
{
    RecordProperty("Requirement", "SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "Ensures that SharedMemoryWriter shall be capable of performing multiple "
        "concurrent write operations and count the total number of write operations successfully, while "
        "correctly verifying that all write operations have been successfully recorded with the correct data.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto type_id = shared_memory_writer_.TryRegisterType(TypeInfoTest{});

    auto call_write_operations = [this, &type_id]() {
        // When writing into it from multiple threads
        std::array<std::thread, kNumberOfThreads> threads{};

        for (std::uint8_t counter{}; counter < threads.size(); counter++)
        {
            threads.at(counter) = std::thread([type_id = type_id.value(), this]() noexcept {
                for (auto number = 0UL; number < kNumberOfActions; number++)
                {
                    shared_memory_writer_.AllocAndWrite(
                        [](auto span) {
                            EXPECT_EQ(span.size(), test_data_sample.size());
                            std::memcpy(span.data(), test_data_sample.data(), test_data_sample.size());
                        },
                        type_id,
                        test_data_sample.size());
                }
            });
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    };

    auto verify_write_operation = [this, &type_id]() {
        auto on_new_type = [&](const score::mw::log::detail::TypeRegistration& registration) noexcept {
            EXPECT_EQ(registration.type_id, type_id.value());
        };
        auto count = 0UL;
        auto on_new_record = [&](const score::mw::log::detail::SharedMemoryRecord& record) noexcept {
            EXPECT_EQ(record.header.type_identifier, type_id.value());
            EXPECT_EQ(record.payload.size(), test_data_sample.size());
            EXPECT_EQ(0, std::memcmp(record.payload.data(), test_data_sample.data(), test_data_sample.size()));
            count++;
        };

        shared_memory_reader_->Read(on_new_type, on_new_record);
        EXPECT_EQ(count, kNumberOfThreads * kNumberOfActions);
    };

    for (auto i = 0UL; i < kNumberOfTests; i++)
    {  //  test write/verify multiple times to test double buffering and to detect synchronization issues:
        call_write_operations();

        const auto read_acquire_result = shared_memory_writer_.ReadAcquire();

        //  Datarouter part after acquisition
        shared_memory_reader_->NotifyAcquisitionSetReader(read_acquire_result);

        verify_write_operation();
    }
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
