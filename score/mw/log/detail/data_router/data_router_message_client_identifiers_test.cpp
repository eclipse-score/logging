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

#include "score/mw/log/detail/data_router/data_router_message_client_identifiers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "score/mw/log/detail/logging_identifier.h"

const std::string kClientReceiverIdentifier = "/logging.app.1234";
const pid_t kThisProcessPid = 99U;
const auto kAppid = score::mw::log::detail::LoggingIdentifier{"TeAp"};
constexpr uid_t kDatarouterDummyUid = 111;
const uid_t kUid = 1234;

namespace score::mw::log::detail
{

namespace
{

TEST(DatarouterMessageClientIdentifiersFixture, TestDataRouterMessageIDsGetters)
{

    MsgClientIdentifiers msg_client_ids(
        kClientReceiverIdentifier, kThisProcessPid, kAppid, static_cast<uid_t>(kDatarouterDummyUid), kUid);

    EXPECT_EQ(msg_client_ids.GetReceiverID(), kClientReceiverIdentifier);
    EXPECT_EQ(msg_client_ids.GetThisProcID(), kThisProcessPid);
    EXPECT_EQ(msg_client_ids.GetAppID(), kAppid);
    EXPECT_EQ(msg_client_ids.GetDatarouterUID(), kDatarouterDummyUid);
    EXPECT_EQ(msg_client_ids.GetUID(), kUid);
}

TEST(DatarouterMessageClientIdentifiersTest, HandlesEmptyReceiverIdentifier)
{
    const MsgClientIdentifiers msg_client_ids("", kThisProcessPid, kAppid, kDatarouterDummyUid, kUid);

    EXPECT_EQ(msg_client_ids.GetReceiverID(), "");
}

TEST(DatarouterMessageClientIdentifiersTest, HandlesZeroPid)
{
    const MsgClientIdentifiers msg_client_ids(kClientReceiverIdentifier, 0, kAppid, kDatarouterDummyUid, kUid);

    EXPECT_EQ(msg_client_ids.GetThisProcID(), 0);
}

TEST(DatarouterMessageClientIdentifiersTest, HandlesMaxUidValues)
{
    constexpr uid_t kMaxUid = std::numeric_limits<uid_t>::max();

    const MsgClientIdentifiers msg_client_ids(kClientReceiverIdentifier, kThisProcessPid, kAppid, kMaxUid, kMaxUid);

    EXPECT_EQ(msg_client_ids.GetDatarouterUID(), kMaxUid);
    EXPECT_EQ(msg_client_ids.GetUID(), kMaxUid);
}

TEST(DatarouterMessageClientIdentifiersTest, HandlesEmptyAppId)
{
    const auto k_empty_app_id = LoggingIdentifier{""};

    const MsgClientIdentifiers msg_client_ids(
        kClientReceiverIdentifier, kThisProcessPid, k_empty_app_id, kDatarouterDummyUid, kUid);

    EXPECT_EQ(msg_client_ids.GetAppID(), k_empty_app_id);
}

}  // namespace
}  // namespace score::mw::log::detail
