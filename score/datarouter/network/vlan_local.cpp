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

#include "score/datarouter/network/vlan_local.h"

#include "score/os/socket.h"

namespace score
{
namespace os
{

namespace
{
/*
Deviation from Rule A16-0-1:
- Rule A16-0-1 (required, implementation, automated)
The pre-processor shall only be used for unconditional and conditional file
inclusion and include guards, and using the following directives: (1) #ifndef,
#ifdef, (3) #if, (4) #if defined, (5) #elif, (6) #else, (7) #define, (8) #endif, (9)
#include.
Justification:
- Feature Flag to os specific definition.
*/
#if defined(__QNX__)
// coverity[autosar_cpp14_a16_0_1_violation] see above
#if __QNX__ >= 800
constexpr auto kVlanPrioOption = -1;  // SO_VLANPRIO is not available in QNX 8.0
// coverity[autosar_cpp14_a16_0_1_violation] see above
#else
constexpr auto kVlanPrioOption = SO_VLANPRIO;
// coverity[autosar_cpp14_a16_0_1_violation] see above
#endif
// coverity[autosar_cpp14_a16_0_1_violation] see above
#else  //__QNX__
constexpr auto kVlanPrioOption = SO_PRIORITY;
// coverity[autosar_cpp14_a16_0_1_violation] see above
#endif

class VlanImpl final : public Vlan
{
    score::cpp::expected_blank<Error> SetVlanPriorityOfSocket(const std::uint8_t pcp_priority,
                                                       const std::int32_t file_descriptor) noexcept override
    {
        return score::os::Socket::instance().setsockopt(
            file_descriptor, SOL_SOCKET, kVlanPrioOption, &pcp_priority, sizeof(pcp_priority));
    }
};

}  // namespace

Vlan& Vlan::instance() noexcept
{
    static VlanImpl instance;
    return select_instance(instance);
}

}  // namespace os
}  // namespace score
