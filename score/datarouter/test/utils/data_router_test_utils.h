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

#ifndef SCORE_PAS_LOGGING_TEST_UTILS_DATA_ROUTER_TEST_UTILS_H
#define SCORE_PAS_LOGGING_TEST_UTILS_DATA_ROUTER_TEST_UTILS_H

#include "score/datarouter/datarouter/data_router.h"

#include "score/static_reflection_with_serialization/serialization/include/serialization/for_logging.h"

namespace test::utils
{

namespace
{
struct TestTypeInfo
{
    std::size_t size() const
    {
        return type_params.size();
    }

    void Copy(const score::cpp::span<score::mw::log::detail::Byte> data) const
    {
        if (score::mw::log::detail::GetDataSizeAsLength(data) != type_params.size())
        {
            // Copy should be called given the same size as returned by size()
            std::abort();
        }
        std::memcpy(data.data(), type_params.data(), type_params.size());
    }

    std::string type_params{};
};

template <typename Message>
TestTypeInfo CreateTypeInfo()
{
    constexpr static std::size_t kIdsize = score::platform::dltid_t::size();
    const std::string app_prefix(kIdsize * 3, char{0});
    TestTypeInfo type_info{};
    type_info.type_params = app_prefix + ::score::common::visitor::logger_type_string<Message>();
    return type_info;
}

}  // namespace
}  // namespace test::utils

#endif  // SCORE_PAS_LOGGING_TEST_UTILS_DATA_ROUTER_TEST_UTILS_H
