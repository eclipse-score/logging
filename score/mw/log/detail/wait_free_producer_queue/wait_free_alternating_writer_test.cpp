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

#include "score/mw/log/detail/wait_free_producer_queue/wait_free_alternating_writer.h"
#include "score/mw/log/detail/wait_free_producer_queue/alternating_reader.h"
#include "score/mw/log/detail/wait_free_producer_queue/alternating_reader_proxy.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace
{

TEST(WaitFreeAlternatingWriterTests, EnsureAtomicRequirements)
{
    RecordProperty("Requirement", "SCR-861578,SCR-1016724,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The used atomic data types shall be lock free");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    score::mw::log::detail::AlternatingControlBlock control_block{};
    ASSERT_TRUE(control_block.switch_count_points_active_for_writing.is_lock_free());
}

TEST(WaitFreeAlternatingWriterTests, WriteBufferFullShouldReturnExpectedData)
{
    RecordProperty("Requirement", "SCR-861578,SCR-1016724,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Producing and consuming of logging data shall work concurrently and without cross-thread locks.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 10u * 64u * 1024u;
    std::vector<score::mw::log::detail::Byte> buffer_even(kBufferSize);
    std::vector<score::mw::log::detail::Byte> buffer_odd(kBufferSize);
    score::mw::log::detail::AlternatingControlBlock control_block{};
    control_block.control_block_even.data =
        score::cpp::span<score::mw::log::detail::Byte>(buffer_even.data(), buffer_even.size());
    control_block.control_block_odd.data = score::cpp::span<score::mw::log::detail::Byte>(buffer_odd.data(), buffer_odd.size());

    score::mw::log::detail::WaitFreeAlternatingWriter writer{InitializeAlternatingControlBlock(control_block)};

    const auto kNumberOfWriterThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(kNumberOfWriterThreads);

    const auto kAcquireLength = kBufferSize / kNumberOfWriterThreads;
    constexpr auto kNumberOfPacketsPerThread = 3;

    for (auto i = 0u; i < kNumberOfWriterThreads; i++)
    {
        threads[i] = std::thread([kAcquireLength, &writer]() {
            for (auto packet_number = 0; packet_number < kNumberOfPacketsPerThread; packet_number++)
            {
                // Loop until we succeeded to reserve data on the buffer
                std::optional<score::mw::log::detail::AlternatingAcquiredData> acquire_result{};

                while (acquire_result.has_value() == false)
                {
                    acquire_result = writer.Acquire(kAcquireLength);
                    std::this_thread::sleep_for(10us);
                }

                auto acquired_data = acquire_result.value().data;
                if (acquired_data.size() != kAcquireLength)
                {
                    std::abort();
                }

                // Write data into the complete acquired span.
                acquired_data.data()[0] = static_cast<score::mw::log::detail::Byte>(packet_number);

                writer.Release(acquire_result.value());
            }
        });
    }

    score::mw::log::detail::AlternatingReaderProxy reader_proxy{control_block};
    score::mw::log::detail::AlternatingReadOnlyReader read_only_reader{control_block, buffer_even, buffer_odd};

    std::vector<std::uint64_t> number_of_packets_received(kNumberOfPacketsPerThread);

    bool all_packets_received = false;
    while (all_packets_received == false)
    {
        //  The responsibility of the switch state monitoring lies in the hands of the ProxyReader:
        const auto acquired = reader_proxy.Switch();
        while (not read_only_reader.IsBlockReleasedByWriters(acquired))
        {
            std::this_thread::sleep_for(10ms);
        }
        //  Terminating block is no longer terminating as all the writers released the buffers.
        //  Buffer is now ready to setup the receiver and read data.
        auto linear_reader = read_only_reader.CreateLinearReader(acquired);
        auto read_result = linear_reader.Read();
        while (read_result.has_value())
        {
            //  cast to a larger type.
            const auto packet_id = static_cast<std::size_t>(*read_result.value().begin());
            number_of_packets_received[packet_id]++;

            read_result = linear_reader.Read();
        }

        all_packets_received = true;
        for (const auto& counter : number_of_packets_received)
        {
            if (counter != kNumberOfWriterThreads)
            {
                all_packets_received = false;
                break;
            }
        }

        std::this_thread::sleep_for(500ms);
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
}

TEST(AlternatingReaderTest, EnsureSafeSwitchingToReadDataBuffer)
{
    RecordProperty("Requirement", "SCR-861578,SCR-1016724,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "Verify the ability to safely switch in case of un-equally index for written_index and acquired_index.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    score::mw::log::detail::AlternatingControlBlock control_block{};

    // Set non-equal values for written_index and acquired_index.
    control_block.control_block_odd.written_index.store(1);
    control_block.control_block_odd.acquired_index.store(0);
    control_block.control_block_even.written_index.store(1);
    control_block.control_block_even.acquired_index.store(0);

    std::ignore = score::mw::log::detail::InitializeAlternatingControlBlock(control_block);

    score::mw::log::detail::AlternatingReaderProxy reader{control_block};

    auto thread = std::thread([&reader]() noexcept {
        // Because we assign defferent values for acquired_index and written_index, the method shoukd sleep for some
        // time without causing the test case to stuck.
        EXPECT_NO_FATAL_FAILURE(reader.Switch());
    });

    // Switch method has a delay in its condition, So. Let's wait some time as in the method itself to make sure the
    // condition satisfied.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // reset the value again to be equally.
    control_block.control_block_odd.written_index.store(0);
    control_block.control_block_even.written_index.store(0);

    thread.join();
}

TEST(AlternatingReaderTest, EnsureSwitchingIncrementsInternalCounter)
{
    RecordProperty("Requirement", "SCR-861578,SCR-1016724,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "Verify the ability to safely switch in case of un-equally index for written_index and acquired_index.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    score::mw::log::detail::AlternatingControlBlock control_block{};
    std::ignore = score::mw::log::detail::InitializeAlternatingControlBlock(control_block);

    score::mw::log::detail::AlternatingReaderProxy reader{control_block};
    score::mw::log::detail::WaitFreeAlternatingWriter writer{control_block};

    EXPECT_EQ(control_block.switch_count_points_active_for_writing, 1UL);
    EXPECT_NO_FATAL_FAILURE(reader.Switch());
    EXPECT_EQ(control_block.switch_count_points_active_for_writing, 2UL);
    EXPECT_NO_FATAL_FAILURE(reader.Switch());
    EXPECT_EQ(control_block.switch_count_points_active_for_writing, 3UL);
}

TEST(AlternatingReaderTest, EnsureToNotAcquireAnyBlockAndReturnNullInCaseTheSwitchingHappenedMoreThanOnce)
{
    RecordProperty("Requirement", "SCR-861578,SCR-1016724,SCR-861550");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verify the ability to not acquiring a block in case of the switch has happened more than once.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = 10u * 64u * 1024u;
    std::vector<score::mw::log::detail::Byte> buffer1(kBufferSize);
    std::vector<score::mw::log::detail::Byte> buffer2(kBufferSize);

    score::mw::log::detail::AlternatingControlBlock control_block{};
    score::mw::log::detail::WaitFreeAlternatingWriter writer{InitializeAlternatingControlBlock(control_block)};

    std::atomic finished_execution{false};
    std::optional<score::mw::log::detail::AlternatingAcquiredData> acquire_result{};
    std::thread thread_instance = std::thread([&writer, &acquire_result, &finished_execution]() noexcept {
        // Delay the start of acquiring to make sure we got inside the below while loop.
        std::this_thread::sleep_for(1s);
        // Set any value to kAcquireLength.
        constexpr auto kAcquireLength = 100000;
        acquire_result = writer.Acquire(kAcquireLength);
        finished_execution = true;
    });

    // We need to make a difference between the two readings of switch_count_points_active_for_writing those inside
    // WaitFreeAlternatingWriter::Acquire and AcquireBlock methods to reach the lines those after the condition
    // statement.
    while (finished_execution == false)
    {
        // Accumulate by 1;
        control_block.switch_count_points_active_for_writing.fetch_add(1);
    }
    thread_instance.join();

    // So, we are expecting no block acquired.
    EXPECT_EQ(acquire_result, std::nullopt);
}

}  // namespace
