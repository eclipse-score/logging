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

#ifndef SCORE_PAS_LOGGING_INCLUDE_DLT_DLTID_H
#define SCORE_PAS_LOGGING_INCLUDE_DLT_DLTID_H

#include "score/mw/log/detail/logging_identifier.h"
#include <cstring>
#include <functional>
#include <iostream>
#include <string>

#include <score/string_view.hpp>

namespace score
{
namespace platform
{
// autosar_cpp14_m3_2_3_violation : A type, object or function that is used in
// multiple translation units shall be declared in one and only one file.
// false positive : dltid_t is only declared once in this file.
// coverity[autosar_cpp14_m3_2_3_violation]
struct dltid_t
{
    constexpr static size_t kSize{4U};

    score::mw::log::detail::LoggingIdentifier bytes;
    std::int32_t value;

    dltid_t() noexcept : value() {}
    explicit dltid_t(const char* str) noexcept : dltid_t(std::string_view{str}) {}
    explicit dltid_t(const std::string_view str) noexcept : bytes{str}, value{}
    {
        value = static_cast<std::int32_t>(score::mw::log::detail::LoggingIdentifier::HashFunction{}(bytes));
    }

    explicit dltid_t(const std::string& str) noexcept : dltid_t(std::string_view{str.data(), str.size()}) {}
    explicit dltid_t(const score::cpp::string_view& str) noexcept : dltid_t(std::string_view{str.data(), str.size()}) {};

    dltid_t& operator=(const std::string& str)
    {
        bytes = score::mw::log::detail::LoggingIdentifier(std::string_view(str));
        value = static_cast<std::int32_t>(score::mw::log::detail::LoggingIdentifier::HashFunction{}(bytes));

        return *this;
    }

    explicit operator std::string() const
    {
        // Determine the length of the C-string stored in bytes,
        // but do not exceed the size of the array.
        return std::string(bytes.data_.data(), bytes.data_.size());
    }

    bool operator==(const dltid_t& id) const
    {
        return this->value == id.value;
    }

    char* data()
    {
        return bytes.data_.data();
    }

    const char* data() const
    {
        return bytes.data_.data();
    }

    constexpr static size_t size()
    {
        return kSize;
    }
};

}  // namespace platform
}  // namespace score

// allowed for hashes for user-defined types
namespace std
{

template <>
// autosar_cpp14_m3_2_3_violation : A type, object or function that is used in
// multiple translation units shall be declared in one and only one file.
// false positive : template specialization for hash.
// coverity[autosar_cpp14_m3_2_3_violation]
struct hash<score::platform::dltid_t>
{
    std::size_t operator()(const score::platform::dltid_t& d) const noexcept
    {
        return static_cast<std::size_t>(d.value);
    }
};

}  // namespace std

#endif  // SCORE_PAS_LOGGING_INCLUDE_DLT_DLTID_H
