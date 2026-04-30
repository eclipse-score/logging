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

#ifndef SCORE_DATAROUTER_INCLUDE_LOGPARSER_I_LOG_PARSER_FACTORY_H
#define SCORE_DATAROUTER_INCLUDE_LOGPARSER_I_LOG_PARSER_FACTORY_H

#include "logparser/i_logparser.h"

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
class NvConfig;
}  // namespace log
}  // namespace mw
namespace platform
{
namespace internal
{

class ILogParserFactory
{
  public:
    virtual ~ILogParserFactory() = default;

    virtual std::unique_ptr<ILogParser> Create(const score::mw::log::NvConfig& nv_config) = 0;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_LOGPARSER_I_LOG_PARSER_FACTORY_H
