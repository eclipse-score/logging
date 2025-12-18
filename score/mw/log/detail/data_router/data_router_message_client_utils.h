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

#ifndef MW_LOG_DETAILS_DATA_ROUTER_MESSAGE_CLIENT_UTILS_H_
#define MW_LOG_DETAILS_DATA_ROUTER_MESSAGE_CLIENT_UTILS_H_

#include "score/os/pthread.h"
#include "score/os/unistd.h"
#include "score/os/utils/signal.h"
#include <iostream>
#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class MsgClientUtils
{
  public:
    MsgClientUtils(score::cpp::pmr::unique_ptr<score::os::Unistd> unistd,
                   score::cpp::pmr::unique_ptr<score::os::Pthread> pthread,
                   score::cpp::pmr::unique_ptr<score::os::Signal> signal);

    score::os::Unistd& GetUnistd() const noexcept;
    score::os::Pthread& GetPthread() const noexcept;
    score::os::Signal& GetSignal() const noexcept;

  private:
    score::cpp::pmr::unique_ptr<score::os::Unistd> unistd_;
    score::cpp::pmr::unique_ptr<score::os::Pthread> pthread_;
    score::cpp::pmr::unique_ptr<score::os::Signal> signal_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // MW_LOG_DETAILS_DATA_ROUTER_MESSAGE_CLIENT_UTILS_H_
