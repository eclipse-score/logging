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

#include "score/mw/log/detail/data_router/data_router_message_client_utils.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

MsgClientUtils::MsgClientUtils(score::cpp::pmr::unique_ptr<score::os::Unistd> unistd,
                               score::cpp::pmr::unique_ptr<score::os::Pthread> pthread,
                               score::cpp::pmr::unique_ptr<score::os::Signal> signal)
    : unistd_{std::move(unistd)}, pthread_{std::move(pthread)}, signal_{std::move(signal)}
{
}

score::os::Unistd& MsgClientUtils::GetUnistd() const noexcept
{
    return *unistd_.get();
}

score::os::Pthread& MsgClientUtils::GetPthread() const noexcept
{
    return *pthread_.get();
}

score::os::Signal& MsgClientUtils::GetSignal() const noexcept
{
    return *signal_.get();
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
