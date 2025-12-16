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

#include "gtest/gtest.h"

#include "score/mw/log/detail/slog/slog_recorder_factory.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

template <typename ConcreteRecorder>
bool IsRecorderOfType(const std::unique_ptr<Recorder>& recorder) noexcept
{
    static_assert(std::is_base_of<Recorder, ConcreteRecorder>::value,
                  "Concrete recorder shall be derived from Recorder base class");

    return dynamic_cast<const ConcreteRecorder*>(recorder.get()) != nullptr;
}

TEST(SlogRecorderFactoryTest, CreateRecorder)
{
    Configuration config;
    score::cpp::pmr::memory_resource* memory_resource = score::cpp::pmr::get_default_resource();

    auto recorder = SlogRecorderFactory{}.CreateConcreteLogRecorder(config, memory_resource);

    // Slog uses TextRecorder
    EXPECT_TRUE(IsRecorderOfType<TextRecorder>(recorder));
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
