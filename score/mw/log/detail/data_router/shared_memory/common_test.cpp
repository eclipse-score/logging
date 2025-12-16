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

#include "score/mw/log/detail/data_router/shared_memory/common.h"

#include "score/optional.hpp"

#include "gtest/gtest.h"

#include <type_traits>

namespace
{

TEST(CommonTests, TypesShallBeTriviallyCopyable)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Verifies the copyable ability of some structs.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ASSERT_TRUE(std::is_trivially_copyable<score::mw::log::detail::BufferEntryHeader>::value);
    ASSERT_TRUE(std::is_trivially_copyable<score::mw::log::detail::ReadAcquireResult>::value);
    ASSERT_TRUE(std::is_trivially_copyable<score::mw::log::detail::TypeIdentifier>::value);
}

TEST(CommonTests, TypesShallBeLockFree)
{
    RecordProperty("ParentRequirement", "SCR-861578");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Checks data lock-free.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    score::mw::log::detail::SharedData data{};
    ASSERT_TRUE(data.number_of_drops_buffer_full.is_lock_free());
    ASSERT_TRUE(data.size_of_drops_buffer_full.is_lock_free());
    ASSERT_TRUE(data.number_of_drops_invalid_size.is_lock_free());
    ASSERT_TRUE(data.writer_detached.is_lock_free());
}

TEST(CommonTests, GetExpectedNextAcquiredBlockId)
{
    {
        score::mw::log::detail::ReadAcquireResult result;
        result.acquired_buffer = 0;

        EXPECT_EQ(score::mw::log::detail::GetExpectedNextAcquiredBlockId(result), result.acquired_buffer + 1);
    }

    {
        score::mw::log::detail::ReadAcquireResult result;
        result.acquired_buffer = 55;

        EXPECT_EQ(score::mw::log::detail::GetExpectedNextAcquiredBlockId(result), result.acquired_buffer + 1);
    }
}

}  // namespace
