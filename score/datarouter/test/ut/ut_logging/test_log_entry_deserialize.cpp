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

#include "score/datarouter/include/daemon/log_entry_deserialization_visitor.h"

#include "static_reflection_with_serialization/serialization/for_logging.h"

#include "score/span.hpp"

#include <vector>

#include "gtest/gtest.h"

using namespace testing;

namespace dlt_server_logging = ::score::common::visitor::dlt_server_logging;

struct SerializedVectorStd
{
    std::uint32_t m;
    std::vector<std::uint8_t> payload;
    std::uint32_t n;
};

STRUCT_TRACEABLE(SerializedVectorStd, m, payload, n)

struct SerializedVectorDataWrapper
{
    std::uint32_t m;
    dlt_server_logging::SerializedVectorData payload;
    std::uint32_t n;
};

STRUCT_TRACEABLE(SerializedVectorDataWrapper, m, payload, n)

namespace test
{

namespace
{

TEST(SerializeDeserialize, DataSerializedFromVectorShouldBeAccessibleUsingSpan)
{
    SerializedVectorStd unit_input;
    unit_input.m = 1;
    unit_input.n = 2;
    unit_input.payload.resize(19);

    std::generate(unit_input.payload.begin(), unit_input.payload.end(), [n = 0]() mutable {
        return ++n;
    });
    SerializedVectorDataWrapper unit_output;

    using S = ::score::common::visitor::logging_serializer;
    std::array<std::uint8_t, 512> buffer_on_stack;
    score::cpp::span serialized_data{buffer_on_stack.begin(), buffer_on_stack.size()};
    const auto ssize =
        S::serialize(unit_input, serialized_data.data(), static_cast<std::uint32_t>(serialized_data.size()));

    S::deserialize(serialized_data.data(), static_cast<std::uint32_t>(serialized_data.size()), unit_output);

    EXPECT_TRUE(ssize > 0);
    EXPECT_EQ(unit_output.m, unit_input.m);
    EXPECT_EQ(unit_output.n, unit_input.n);
    EXPECT_EQ(unit_output.payload.data.size(), unit_input.payload.size());

    auto out = unit_output.payload.data.begin();
    auto index = 0UL;
    for (const auto& in : unit_input.payload)
    {
        EXPECT_EQ(in, *out);
        out++;
        index++;
    }
    //  Defensive programming: confirm range going over each element:
    EXPECT_EQ(index, unit_input.payload.size());
}

TEST(SerializeDeserialize, DataSerializedFromDeserializeOffsetReturnZero)
{
    SerializedVectorDataWrapper unit_output{};

    using S = ::score::common::visitor::logging_serializer;
    std::array<std::uint8_t, 512> buffer_on_stack{};
    score::cpp::span serialized_data{buffer_on_stack.begin(), buffer_on_stack.size()};

    S::deserialize(serialized_data.data(), static_cast<std::uint32_t>(serialized_data.size()), unit_output);

    EXPECT_EQ(unit_output.n, 0);
}

TEST(SerializeDeserialize, DataSerializedFromDeserializeVectorSizeLocationNullptr)
{
    SerializedVectorDataWrapper unit_output{};

    using S = ::score::common::visitor::logging_serializer;
    std::array<std::uint8_t, 512> buffer_on_stack{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    score::cpp::span serialized_data{buffer_on_stack.begin(), buffer_on_stack.size()};

    S::deserialize(serialized_data.data(), static_cast<std::uint32_t>(serialized_data.size()), unit_output);

    // Should have some garbage values, because initial buffer contains garbage.
    EXPECT_NE(unit_output.m, 0);
    EXPECT_NE(unit_output.n, 0);
}

TEST(SerializeDeserialize, DataSerializedFromDeserializeVectorSizeLocationZero2)
{
    SerializedVectorStd unit_input;
    unit_input.m = 1;
    unit_input.n = 2;
    unit_input.payload.resize(5);

    std::generate(unit_input.payload.begin(), unit_input.payload.end(), [n = 0]() mutable {
        return n;
    });
    SerializedVectorDataWrapper unit_output{};

    using S = ::score::common::visitor::logging_serializer;
    std::array<std::uint8_t, 512> buffer_on_stack{};
    std::generate(buffer_on_stack.begin(), buffer_on_stack.end(), [n = 255]() mutable {
        return n;
    });
    score::cpp::span serialized_data{buffer_on_stack.begin(), buffer_on_stack.size()};
    const auto ssize =
        S::serialize(unit_input, serialized_data.data(), static_cast<std::uint32_t>(serialized_data.size()));

    // corrupt serialized buffer
    auto it = serialized_data.begin();
    std::advance(it, static_cast<std::string::difference_type>(2));
    auto* src = std::addressof(*it);
    std::array<std::uint8_t, 4> garbage{1, 1, 1, 1};
    memcpy(src, garbage.begin(), garbage.size());

    S::deserialize(serialized_data.data(), static_cast<std::uint32_t>(serialized_data.size()), unit_output);

    EXPECT_TRUE(ssize > 0);
    // Should have some garbage values, because initial buffer corrupted.
    EXPECT_NE(unit_output.m, 0);
    EXPECT_NE(unit_output.n, 0);
}

}  // namespace

}  // namespace test
