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

#ifndef SCORE_DATAROUTER_MOCKS_LOGPARSER_MOCK_H
#define SCORE_DATAROUTER_MOCKS_LOGPARSER_MOCK_H

#include "logparser/i_logparser.h"

#include <functional>
#include <optional>
#include <string>

#include <gmock/gmock.h>

namespace score
{
namespace platform
{
namespace internal
{

class LogParserMock : public ILogParser
{
  public:
    ~LogParserMock() = default;

    MOCK_METHOD(void, SetFilterFactory, (FilterFunctionFactory factory), (override));

    MOCK_METHOD(void, AddIncomingType, (const BufsizeT map_index, const std::string& params), (override));
    MOCK_METHOD(void, AddIncomingType, (const score::mw::log::detail::TypeRegistration&), (override));

    MOCK_METHOD(void, AddTypeHandler, (const std::string& type_name, TypeHandler& handler), (override));
    MOCK_METHOD(void, AddGlobalHandler, (AnyHandler & handler), (override));

    MOCK_METHOD(void, RemoveTypeHandler, (const std::string& type_name, TypeHandler& handler), (override));
    MOCK_METHOD(void, RemoveGlobalHandler, (AnyHandler & handler), (override));

    MOCK_METHOD(bool, IsTypeHndlRegistered, (const std::string& type_name, const TypeHandler& handler), (override));
    MOCK_METHOD(bool, IsGlbHndlRegistered, (const AnyHandler& handler), (override));

    MOCK_METHOD(void, ResetInternalMapping, (), (override));
    MOCK_METHOD(void, Parse, (TimestampT timestamp, const char* data, BufsizeT size), (override));
    MOCK_METHOD(void, Parse, (const score::mw::log::detail::SharedMemoryRecord& record), (override));
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_MOCKS_LOGPARSER_MOCK_H
