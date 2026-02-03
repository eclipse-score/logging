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

#include "score/datarouter/src/persistent_logging/persistent_logging_stub/stub_sysedr_handler.h"

namespace score
{
namespace platform
{
namespace internal
{

// LCOV_EXCL_START (nothing to test because there is no implementation)
// LogParser::TypeHandler
void StubSysedrHandler::handle(timestamp_t /* timestamp */, const char* data, bufsize_t size) {}

// LogParser::AnyHandler
void StubSysedrHandler::handle(const TypeInfo& type_info, timestamp_t timestamp, const char* data, bufsize_t size) {}

// LCOV_EXCL_STOP

}  // namespace internal
}  // namespace platform
}  // namespace score
