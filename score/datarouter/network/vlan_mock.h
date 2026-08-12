/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
#ifndef SCORE_DATAROUTER_NETWORK_VLAN_MOCK_H
#define SCORE_DATAROUTER_NETWORK_VLAN_MOCK_H

#include "score/os/errno.h"
#include "score/network/vlan.h"

#include <gmock/gmock.h>

namespace score
{
namespace os
{

class VlanMock : public ::score::os::Vlan
{
  public:
    MOCK_METHOD((score::cpp::expected_blank<::score::os::Error>),
                SetVlanPriorityOfSocket,
                (const std::uint8_t, const std::int32_t),
                (noexcept, override));
};

}  // namespace os
}  // namespace score

#endif  // SCORE_DATAROUTER_NETWORK_VLAN_MOCK_H
