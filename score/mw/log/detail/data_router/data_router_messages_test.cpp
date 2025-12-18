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

#include "score/mw/log/detail/data_router/data_router_messages.h"

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

const auto kAppid = LoggingIdentifier{"TeAp"};
const auto kAppid2 = LoggingIdentifier{"TEAp"};
const uid_t kUid = 1234;
const uid_t kUid2 = 4321;
const bool kDynamicDatarouterIdentifiersFalse = false;
const bool kDynamicDatarouterIdentifiersTrue = true;
std::array<char, 6> random_part1{"x"};
std::array<char, 6> random_part2{"y"};

TEST(DataRouterMessagesTests, EqualOperatorShouldReturnTrueForEqualConnectMessageFromClientInstances)
{
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "Checks the equal operator of ConnectMessageFromClient for equaly ConenctMessageFromClients instances.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient lhs{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};
    ConnectMessageFromClient rhs{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};
    EXPECT_EQ(lhs, rhs);
}

TEST(DataRouterMessagesTests, EqualOperatorShouldReturnFalseForDifferentAppids)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Checks the equal operator of ConnectMessageFromClient for unequal App ids.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient lhs{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};
    ConnectMessageFromClient rhs{kAppid2, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};
    EXPECT_NE(lhs, rhs);
}

TEST(DataRouterMessagesTests, EqualOperatorShouldReturnFalseForDifferentUids)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Checks the equal operator of ConnectMessageFromClient for unequal uids.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient lhs{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};
    ConnectMessageFromClient rhs{kAppid, kUid2, kDynamicDatarouterIdentifiersFalse, random_part1};
    EXPECT_NE(lhs, rhs);
}

TEST(DataRouterMessagesTests, EqualOperatorShouldReturnFalseForDifferentDynamicDatarouterIdentifiers)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Checks the equal operator of ConnectMessageFromClient for unequal dynamic identifiers.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient lhs{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};
    ConnectMessageFromClient rhs{kAppid, kUid, kDynamicDatarouterIdentifiersTrue, random_part1};
    EXPECT_NE(lhs, rhs);
}

TEST(DataRouterMessagesTests, EqualOperatorShouldReturnFalseForDifferentRandomPart)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Checks the equal operator of ConnectMessageFromClient for unequal random parts.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient lhs{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};
    ConnectMessageFromClient rhs{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part2};
    EXPECT_NE(lhs, rhs);
}

TEST(DataRouterMessagesTests, GetGUIShouldReturnCorrectValue)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Checks GetUid function of ConnectMessageFromClient return correct value.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient message{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};

    EXPECT_EQ(message.GetUid(), kUid);

    message.SetUid(kUid2);

    EXPECT_EQ(message.GetUid(), kUid2);
}

TEST(DataRouterMessagesTests, GetUseDynamicIdentifierShouldReturnCorrectValue)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description",
                   "Checks GetUseDynamicIdentifier function of ConnectMessageFromClient return correct value.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient message{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};

    EXPECT_EQ(message.GetUseDynamicIdentifier(), kDynamicDatarouterIdentifiersFalse);

    message.SetUseDynamicIdentifier(kDynamicDatarouterIdentifiersTrue);

    EXPECT_EQ(message.GetUseDynamicIdentifier(), kDynamicDatarouterIdentifiersTrue);
}

TEST(DataRouterMessagesTests, GetAppIdDynamicIdentifierShouldReturnCorrectValue)
{
    RecordProperty("ASIL", "B");
    RecordProperty("Description", "Checks GetAppId function of ConnectMessageFromClient return correct value.");
    RecordProperty("TestType", "Interface test");
    RecordProperty("DerivationTechnique", "Generation and analysis of equivalence classes");

    ConnectMessageFromClient message{kAppid, kUid, kDynamicDatarouterIdentifiersFalse, random_part1};

    EXPECT_EQ(message.GetAppId(), kAppid);

    message.SetAppId(kAppid2);

    EXPECT_EQ(message.GetAppId(), kAppid2);
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
