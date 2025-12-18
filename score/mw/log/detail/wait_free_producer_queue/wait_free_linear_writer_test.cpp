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

#include "score/mw/log/detail/wait_free_producer_queue/wait_free_linear_writer.h"
#include "score/mw/log/detail/wait_free_producer_queue/linear_reader.h"

#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <thread>
#include <vector>

namespace
{

TEST(WaitFreeLinearWriter, EnsureAtomicRequirements)
{
    RecordProperty("Requirement", "SCR-861578, SCR-1016724");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The used atomic data types shall be lock free");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    score::mw::log::detail::LinearControlBlock control_block{};
    ASSERT_TRUE(control_block.acquired_index.is_lock_free());
    ASSERT_TRUE(control_block.number_of_writers.is_lock_free());
    ASSERT_TRUE(control_block.written_index.is_lock_free());
}

TEST(WaitFreeLinearWriter, WriteBufferFullShouldReturnExpectedData)
{
    RecordProperty("Requirement", "SCR-861578, SCR-1016724, SCR-1016719, SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Returning the expected data if the write buffer is full.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 10u * 64u * 1024u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());

    score::mw::log::detail::WaitFreeLinearWriter writer{control_block};

    const auto kNumberOfWriterThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(kNumberOfWriterThreads);

    const auto kThreadAcquireFactor = kBufferSize / kNumberOfWriterThreads;

    for (auto i = 0u; i < kNumberOfWriterThreads; i++)
    {
        threads[i] = std::thread([i, kThreadAcquireFactor, &writer]() {
            const auto kAcquireLength = kThreadAcquireFactor * i;

            const auto acquire_result = writer.Acquire(kAcquireLength);

            if (acquire_result.has_value() == false)
            {
                return;
            }

            // Write data into the complete acquired span.
            const auto acquired_data = acquire_result.value().data;

            if (acquired_data.size() != kAcquireLength)
            {
                std::abort();
            }

            for (auto payload_index = 0u; payload_index < acquired_data.size(); payload_index++)
            {
                acquired_data.data()[payload_index] = static_cast<score::mw::log::detail::Byte>(payload_index);
            }

            writer.Release(acquire_result.value());
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    auto reader = score::mw::log::detail::CreateLinearReaderFromControlBlock(control_block);

    while (true)
    {
        const auto read_result = reader.Read();
        if (read_result.has_value() == false)
        {
            break;
        }

        for (auto payload_index = 0u; payload_index < read_result.value().size(); payload_index++)
        {
            ASSERT_EQ(read_result.value().data()[payload_index],
                      static_cast<score::mw::log::detail::Byte>(payload_index));
        }
    }
}

TEST(WaitFreeLinearWriter, WriterBigDataTest)
{
    RecordProperty("Requirement", "SCR-861578, SCR-1016724, SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the ability of writing big data.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 10u * 10u * 64u * 1024u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());

    score::mw::log::detail::WaitFreeLinearWriter writer{control_block};

    const auto kNumberOfWriterThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(kNumberOfWriterThreads);

    const auto kThreadAcquireFactor = kBufferSize / kNumberOfWriterThreads;

    for (auto i = 0u; i < kNumberOfWriterThreads; i++)
    {
        threads[i] = std::thread([i, kThreadAcquireFactor, &writer]() {
            const auto kAcquireLength = kThreadAcquireFactor * i;

            const auto acquire_result = writer.Acquire(kAcquireLength);

            if (acquire_result.has_value() == false)
            {
                return;
            }

            const auto acquired_data = acquire_result.value().data;

            if (acquired_data.size() != kAcquireLength)
            {
                std::abort();
            }

            // Only write at the beginning and the end as writing everywhere would be too slow.
            if (acquired_data.size() >= 2)
            {
                *acquired_data.begin() = 1;
                acquired_data.data()[acquired_data.size() - 1] = 2;
            }

            writer.Release(acquire_result.value());
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    auto reader = score::mw::log::detail::CreateLinearReaderFromControlBlock(control_block);

    while (true)
    {
        const auto read_result = reader.Read();
        if (read_result.has_value() == false)
        {
            break;
        }

        // We don't check data here as this would be super slow under memcheck configuration.
        if (read_result.value().size() >= 2)
        {
            ASSERT_EQ(read_result.value().data()[0], 1);
            ASSERT_EQ(read_result.value().data()[read_result.value().size() - 1], 2);
        }
    }
}

TEST(WaitFreeLinearWriter, TooManyConcurrentWriterShouldReturnEmpty)
{
    RecordProperty("Requirement", "SCR-861578, SCR-1016724, SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Shall return empty in case of too many concurrent writers.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 10u * 10u * 64u * 1024u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());
    control_block.number_of_writers.store(score::mw::log::detail::GetMaxNumberOfConcurrentWriters());

    score::mw::log::detail::WaitFreeLinearWriter writer{control_block};
    constexpr auto kArbitraryNumberOfBytes{42u};
    ASSERT_FALSE(writer.Acquire(kArbitraryNumberOfBytes).has_value());
}

TEST(WaitFreeLinearWriter, BufferSizeExceededShouldReturnEmpty)
{
    RecordProperty("Requirement", "SCR-861578, SCR-1016724, SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Shall return empty if buffer size exceeded.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 10u * 10u * 64u * 1024u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());
    control_block.acquired_index.store(static_cast<score::mw::log::detail::Length>(control_block.data.size()));

    score::mw::log::detail::WaitFreeLinearWriter writer{control_block};
    constexpr auto kArbitraryNumberOfBytes{42u};
    ASSERT_FALSE(writer.Acquire(kArbitraryNumberOfBytes).has_value());
}

TEST(WaitFreeLinearWriter, BufferSizeExceededUpperLimitShouldReturnEmpty)
{
    RecordProperty("Requirement", "SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Acquire shall fail if buffer size exceeded upper limit.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 10u * 10u * 64u * 1024u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());
    control_block.acquired_index.store(score::mw::log::detail::GetMaxLinearBufferCapacityBytes());

    score::mw::log::detail::WaitFreeLinearWriter writer{control_block};
    constexpr auto kArbitraryNumberOfBytes{42u};
    ASSERT_FALSE(writer.Acquire(kArbitraryNumberOfBytes).has_value());
}

TEST(WaitFreeLinearWriter, FailedAcquireShouldTerminateBuffer)
{
    RecordProperty("Requirement", "SCR-1016719, SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "If the linear buffer is full, acquire shall fail. In case of failed acquisition, the writer shall "
                   "at least write the length if sufficient space is available.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = score::mw::log::detail::GetLengthOffsetBytes() * 2u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());

    score::mw::log::detail::AcquiredData acquired_data{};

    score::mw::log::detail::WaitFreeLinearWriter writer(
        control_block, [i = 0, &acquired_data](score::mw::log::detail::WaitFreeLinearWriter& writer_callback) mutable {
            // Simulate the case where another writer concurrently steals the capacity just before the current
            // thread tries to reserve it.
            if (i++ == 0)
            {
                auto acquire_result = writer_callback.Acquire(0u);
                if (acquire_result.has_value() == false)
                {
                    std::abort();
                }
                acquired_data = acquire_result.value();
            }
        });

    ASSERT_FALSE(writer.Acquire(score::mw::log::detail::GetLengthOffsetBytes()).has_value());
    writer.Release(acquired_data);

    // acquired_index should be equal to the length of the first acquire plus the second acquire including overhead for
    // length.
    const auto expected_acquired_index =
        score::mw::log::detail::GetLengthOffsetBytes() + 2 * score::mw::log::detail::GetLengthOffsetBytes();
    ASSERT_EQ(control_block.acquired_index, expected_acquired_index);
    ASSERT_EQ(control_block.written_index, control_block.acquired_index);

    auto reader = score::mw::log::detail::CreateLinearReaderFromControlBlock(control_block);

    auto read_result = reader.Read();
    ASSERT_TRUE(read_result.has_value());
    ASSERT_EQ(read_result.value().size(), 0);
    ASSERT_FALSE(reader.Read().has_value());
}

TEST(WaitFreeLinearWriter, FailedAcquireWithNoFreeSpaceShouldNotTerminateBuffer)
{
    RecordProperty("Requirement", "SCR-1016719, SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "If the linear buffer is full, acquire shall fail. In a failed acquisition case, the writer should "
                   "not write the length if there is no sufficient space left.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = score::mw::log::detail::GetLengthOffsetBytes() * 2u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());

    score::mw::log::detail::AcquiredData acquired_data{};

    score::mw::log::detail::WaitFreeLinearWriter writer(
        control_block, [i = 0, &acquired_data](score::mw::log::detail::WaitFreeLinearWriter& writer_callback) mutable {
            // Simulate the case where another writer concurrently steals the capacity just before the current
            // thread tries to reserve it.
            if (i++ == 0)
            {
                auto acquire_result = writer_callback.Acquire(score::mw::log::detail::GetLengthOffsetBytes());
                if (acquire_result.has_value() == false)
                {
                    std::abort();
                }
                acquired_data = acquire_result.value();
            }
        });

    ASSERT_FALSE(writer.Acquire(score::mw::log::detail::GetLengthOffsetBytes()).has_value());
    writer.Release(acquired_data);

    // acquired_index should be equal to the length of the first acquire plus the second acquire including overhead for
    // length.
    const auto expected_acquired_index =
        2 * score::mw::log::detail::GetLengthOffsetBytes() + 2 * score::mw::log::detail::GetLengthOffsetBytes();
    ASSERT_EQ(control_block.acquired_index, expected_acquired_index);
    ASSERT_EQ(control_block.written_index, control_block.acquired_index);

    auto reader = score::mw::log::detail::CreateLinearReaderFromControlBlock(control_block);

    auto read_result = reader.Read();
    ASSERT_TRUE(read_result.has_value());
    ASSERT_EQ(read_result.value().size(), score::mw::log::detail::GetLengthOffsetBytes());
    ASSERT_FALSE(reader.Read().has_value());
}

TEST(WaitFreeLinearWriter, AcquireMoreThanMaximumShouldFail)
{
    RecordProperty("Requirement", "SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Acquiring more than the supported threshold shall fail.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = score::mw::log::detail::GetLengthOffsetBytes() * 2u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    score::mw::log::detail::LinearControlBlock control_block{};
    control_block.data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(), buffer.size());

    score::mw::log::detail::WaitFreeLinearWriter writer(control_block);

    ASSERT_FALSE(writer.Acquire(score::mw::log::detail::GetMaxAcquireLengthBytes() + 1).has_value());
}

}  // namespace
