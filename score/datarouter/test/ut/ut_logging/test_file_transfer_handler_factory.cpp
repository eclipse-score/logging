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

#include "score/datarouter/include/applications/datarouter_feature_config.h"
#include "score/datarouter/src/file_transfer/file_transfer_handler_factory.hpp"

#if defined(DLT_FILE_TRANSFER_FEATURE)
#include "score/datarouter/src/file_transfer/file_transfer_impl/file_transfer_stream_handler_factory.h"
#else
#include "score/datarouter/src/file_transfer/file_transfer_stub/file_transfer_handler_factory_stub.h"
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

namespace score
{
namespace logging
{
namespace dltserver
{
namespace
{

// For easy calling FileTransferStreamHandlerType.
using namespace score::platform::datarouter;

template <typename ConcreteStreamHandler>
bool IsFileTransferOfType(const std::unique_ptr<LogParser::TypeHandler>& stream_handler) noexcept
{
    static_assert(std::is_base_of<LogParser::TypeHandler, ConcreteStreamHandler>::value,
                  "Concrete stream handler shall be derived from LogParser::TypeHandler base class");

    return dynamic_cast<const ConcreteStreamHandler*>(stream_handler.get()) != nullptr;
}

// We are testing the CRTP factory, we need to make sure that the stub can properly created when disabling
// the feature flag.
TEST(FileTransferStreamHandlerFactoryTest,
     CreateWithFileTransferFeatureEnabledShallReturnConcreteFileTransferStreamHandler)
{

    Output output;
#if defined(DLT_FILE_TRANSFER_FEATURE)
    FileTransferStreamHandlerFactory factory(output);
#else
    StubFileTransferHandlerFactory factory(output);
#endif
    std::unique_ptr<LogParser::TypeHandler> stream_handler = factory.create();

    // Check that the returned unique_ptr is not null
    ASSERT_NE(stream_handler, nullptr);

    EXPECT_TRUE(IsFileTransferOfType<FileTransferStreamHandlerType>(stream_handler));
}

}  // namespace
}  // namespace dltserver
}  // namespace logging
}  // namespace score
