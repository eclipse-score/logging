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

#ifndef SCORE_PAS_LOGGING_SRC_PERSISTENT_LOGGING_ISYSEDR_HANDLER_H
#define SCORE_PAS_LOGGING_SRC_PERSISTENT_LOGGING_ISYSEDR_HANDLER_H

#include "logparser/logparser.h"

// LCOV_EXCL_START (nothing to test because this is just an interface definition)
namespace score
{
namespace platform
{
namespace internal
{

class ISysedrHandler : public LogParser::TypeHandler, public LogParser::AnyHandler
{
  public:
    ISysedrHandler() = default;
    ~ISysedrHandler() = default;

  protected:
    // LogParser::TypeHandler
    void handle(timestamp_t /* timestamp */, const char* /*data*/, bufsize_t /*size*/) override {};

    // LogParser::AnyHandler
    void handle(const TypeInfo& /*typeInfo*/,
                timestamp_t /*timestamp*/,
                const char* /*data*/,
                bufsize_t /*size*/) override {};
};

// LCOV_EXCL_STOP

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_SRC_PERSISTENT_LOGGING_ISYSEDR_HANDLER_H
