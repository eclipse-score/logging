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

#include "score/mw/log/detail/wait_free_producer_queue/alternating_control_block.h"

#include <gtest/gtest.h>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
namespace test
{

namespace
{

TEST(AlternatingControlBlockTest, GettingOppositeBlockShallSucceed)
{
    RecordProperty("Requirement", "SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The helper function shall check if the length and offset are valid");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_EQ(GetOppositeLinearControlBlock(AlternatingControlBlockSelectId::kBlockEven),
              AlternatingControlBlockSelectId::kBlockOdd);
    EXPECT_EQ(GetOppositeLinearControlBlock(AlternatingControlBlockSelectId::kBlockOdd),
              AlternatingControlBlockSelectId::kBlockEven);
}

TEST(AlternatingControlBlockTest, GettingBlockEvenAndOddBasedOnCoutnerValue)
{
    RecordProperty("Requirement", "SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The helper function shall check if the length and offset are valid");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_EQ(SelectLinearControlBlockId(1UL), AlternatingControlBlockSelectId::kBlockOdd);
    EXPECT_EQ(SelectLinearControlBlockId(3UL), AlternatingControlBlockSelectId::kBlockOdd);

    EXPECT_EQ(SelectLinearControlBlockId(0UL), AlternatingControlBlockSelectId::kBlockEven);
    EXPECT_EQ(SelectLinearControlBlockId(2UL), AlternatingControlBlockSelectId::kBlockEven);
}

TEST(AlternatingControlBlockTest, GettingReferenceBlock)
{
    RecordProperty("Requirement", "SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The helper function shall check if the length and offset are valid");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const AlternatingControlBlock const_block;
    EXPECT_EQ(&SelectLinearControlBlockReference(AlternatingControlBlockSelectId::kBlockEven, const_block),
              &const_block.control_block_even);
    EXPECT_EQ(&SelectLinearControlBlockReference(AlternatingControlBlockSelectId::kBlockOdd, const_block),
              &const_block.control_block_odd);
}

TEST(AlternatingControlBlockTest, GettingReferenceConstBlock)
{
    RecordProperty("Requirement", "SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "The helper function shall check if the length and offset are valid");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    AlternatingControlBlock block;
    EXPECT_EQ(&SelectLinearControlBlockReference(AlternatingControlBlockSelectId::kBlockEven, block),
              &block.control_block_even);
    EXPECT_EQ(&SelectLinearControlBlockReference(AlternatingControlBlockSelectId::kBlockOdd, block),
              &block.control_block_odd);
}

TEST(AlternatingControlBlockTest, GetOppositeLinearControlBlockShallReturnkBlockEvenAsDefaultValue)
{
    RecordProperty("DerivationTechnique", "Analysis of boundary values");
    RecordProperty("TestingTechnique", "Interface tests");
    RecordProperty("Verifies",
                   "score::mw::log::detail::wait_free_producer_queue::alternating_control_block_test::"
                   "GetOppositeLinearControlBlock");
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Verifies the ability of returning the default value in case of passing unknown value to "
                   "GetOppositeLinearControlBlock.");

    // When setting a value outside the listed enum values.
    enum ExtendAlternatingControlBlockSelectId : std::underlying_type_t<AlternatingControlBlockSelectId>
    {
        kExtendedBlockEven = static_cast<std::underlying_type_t<AlternatingControlBlockSelectId>>(
            AlternatingControlBlockSelectId::kBlockOdd),
        kDefault,
    };
    AlternatingControlBlockSelectId new_alternating_control_block_select_id =
        static_cast<AlternatingControlBlockSelectId>(ExtendAlternatingControlBlockSelectId::kDefault);

    // We expecting GetOppositeLinearControlBlock returns kBlockEven as a default value.
    EXPECT_EQ(GetOppositeLinearControlBlock(new_alternating_control_block_select_id),
              AlternatingControlBlockSelectId::kBlockEven);
}

}  // namespace

}  // namespace test
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
