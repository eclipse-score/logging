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

#ifndef SCORE_DATAROUTER_INCLUDE_DLT_DLTID_H
#define SCORE_DATAROUTER_INCLUDE_DLT_DLTID_H

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <string>

#include "score/mw/log/detail/logging_identifier.h"
#include <score/string_view.hpp>

namespace score
{
namespace platform
{
// autosar_cpp14_m3_2_3_violation : A type, object or function that is used in
// multiple translation units shall be declared in one and only one file.
// false positive : dltid_t is only declared once in this file.
// coverity[autosar_cpp14_m3_2_3_violation]
class DltidT
{
  private:
    constexpr static size_t kSize{4U};

    std::array<char, kSize> bytes_;

  public:
    DltidT() noexcept : bytes_{'\0', '\0', '\0', '\0'} {}

    // class data members have been initialized indirectly via delegating constructor.
    // coverity[autosar_cpp14_a12_6_1_violation]: Explanation above.
    explicit DltidT(const std::string_view str) noexcept : DltidT()
    {
        const auto len = std::min(str.size(), kSize);
        std::ignore = std::copy_n(str.begin(), len, bytes_.begin());
    }

    explicit DltidT(const char* str) noexcept : DltidT(std::string_view{str}) {}

    explicit DltidT(const std::string& str) noexcept : DltidT(std::string_view{str}) {}

    explicit DltidT(const score::mw::log::detail::LoggingIdentifier& logging_identifier) noexcept
        : DltidT{logging_identifier.GetStringView()}
    {
    }

    DltidT& operator=(const std::string& str) noexcept
    {
        bytes_.fill('\0');
        const auto len = std::min(str.size(), kSize);
        std::ignore = std::copy_n(str.data(), len, bytes_.begin());
        return *this;
    }

    explicit operator std::string() const
    {
        const auto actual_len = std::count_if(bytes_.begin(), bytes_.end(), [](char byte) {
            return byte != '\0';
        });
        return std::string(bytes_.data(), static_cast<std::size_t>(actual_len));
    }

    friend bool operator==(const DltidT& lhs, const DltidT& rhs) noexcept
    {
        return lhs.bytes_ == rhs.bytes_;
    }

    std::string_view Data() const noexcept
    {
        const auto actual_len = std::count_if(bytes_.begin(), bytes_.end(), [](char byte) {
            return byte != '\0';
        });
        return std::string_view{bytes_.data(), static_cast<std::size_t>(actual_len)};
    }

    constexpr static size_t size()
    {
        return kSize;
    }

    std::int32_t GetHash() const noexcept
    {
        const auto value = score::cpp::bit_cast<std::int32_t>(bytes_);
        return static_cast<std::int32_t>(std::hash<std::int32_t>{}(value));
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
struct hash<score::platform::DltidT>
{
    std::size_t operator()(const score::platform::DltidT& d) const noexcept
    {
        return static_cast<std::size_t>(d.GetHash());
    }
};

}  // namespace std

#endif  // SCORE_DATAROUTER_INCLUDE_DLT_DLTID_H
