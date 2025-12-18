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

#include "score/datarouter/include/daemon/priority_boost.h"

#include "score/os/pthread.h"

#include <cerrno>
#include <iostream>

namespace score
{
namespace logging
{
namespace dltserver
{

PriorityBoost::PriorityBoost(const std::int32_t priority, const std::int32_t policy, score::os::Pthread& pthread) noexcept
    : pthread_{pthread}, thread_{pthread.self()}, old_sched_params_{}, old_policy_{}, priority_boosted_{false}
{
    const auto get_schedparam_result = pthread_.pthread_getschedparam(thread_, &old_policy_, &old_sched_params_);
    if (get_schedparam_result.has_value() == false)
    {
        std::cout << "pthread_getschedparam() failed to get old priority: " << get_schedparam_result.error().ToString()
                  << std::endl;
        return;
    }

    sched_param sched_params_boost{};
    sched_params_boost.sched_priority = priority;
    const auto boost_result = pthread_.pthread_setschedparam(thread_, policy, &sched_params_boost);
    if (boost_result.has_value() == false)
    {
        std::cout << "pthread_setschedparam() failed to set boost priority: " << boost_result.error().ToString()
                  << std::endl;
        return;
    }

    sched_param current_sched_params{};

    std::int32_t current_policy{};

    const auto current_schedparam_result =
        pthread_.pthread_getschedparam(thread_, &current_policy, &current_sched_params);
    if (current_schedparam_result.has_value() == false)
    {
        std::cout << "pthread_getschedparam() failed to get current priority: "
                  << current_schedparam_result.error().ToString() << std::endl;
        return;
    }

    if (current_policy != policy)
    {
        std::cout << "current_policy != policy: " << current_policy << "!=" << policy << std::endl;
    }
    if (current_sched_params.sched_priority != priority)
    {
        std::cout << "current_priority != priority: " << current_sched_params.sched_priority << "!=" << priority
                  << std::endl;
    }

    priority_boosted_ = true;
}

PriorityBoost::~PriorityBoost() noexcept
{
    if (priority_boosted_ == false)
    {
        return;
    }

    // Reset old priority.
    const auto reset_result = pthread_.pthread_setschedparam(thread_, old_policy_, &old_sched_params_);
    if (reset_result.has_value() == false)
    {
        std::cout << "pthread_setschedparam() failed to reset old priority: " << reset_result.error().ToString()
                  << std::endl;
    }
}

}  // namespace dltserver
}  // namespace logging
}  // namespace score
