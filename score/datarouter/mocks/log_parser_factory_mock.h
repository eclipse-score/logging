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

#ifndef SCORE_DATAROUTER_MOCKS_LOG_PARSER_FACTORY_MOCK_H
#define SCORE_DATAROUTER_MOCKS_LOG_PARSER_FACTORY_MOCK_H

#include "logparser/i_log_parser_factory.h"

#include <gmock/gmock.h>

namespace score
{
namespace platform
{
namespace internal
{

class ILogParserFactoryMock : public ILogParserFactory
{
  public:
    ~ILogParserFactoryMock() override = default;

    MOCK_METHOD(std::unique_ptr<ILogParser>, Create, (const score::mw::log::NvConfig& nv_config), (override));
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_MOCKS_LOG_PARSER_FACTORY_MOCK_H
