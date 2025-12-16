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

/// Only testing additional corner cases here, the Reader is mostly already tested through behavior tests in
/// wait_free_linear_writer.cpp.

#include "score/mw/log/detail/wait_free_producer_queue/linear_reader.h"

#include <gtest/gtest.h>

#include <vector>

namespace
{

TEST(LinearReaderTests, LengthExceedingMaxThresholdShouldReturnEmpty)
{
    RecordProperty("Requirement", "SCR-1016719");
    RecordProperty("ASIL", "B");
    RecordProperty(
        "Description",
        "The reader shall check if the length is valid and drop values that would lead to out of bounds access.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    constexpr auto kBufferSize = score::mw::log::detail::GetLengthOffsetBytes() * 2u;
    std::vector<score::mw::log::detail::Byte> buffer(kBufferSize);
    auto data = score::cpp::span<score::mw::log::detail::Byte>(buffer.data(),
                                                      static_cast<score::mw::log::detail::SpanLength>(buffer.size()));
    const auto invalid_length = score::mw::log::detail::GetMaxAcquireLengthBytes() + 1;
    std::ignore = memcpy(buffer.data(), &invalid_length, sizeof(invalid_length));

    score::mw::log::detail::LinearReader reader(data);
    ASSERT_FALSE(reader.Read().has_value());
}

}  // namespace
