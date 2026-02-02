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

#ifndef SCORE_PAS_LOGGING_SRC_PERSISTENT_LOGGING_PERSISTENT_LOGGING_STUB_STUB_SYSEDR_FACTORY_H
#define SCORE_PAS_LOGGING_SRC_PERSISTENT_LOGGING_PERSISTENT_LOGGING_STUB_STUB_SYSEDR_FACTORY_H

#include "score/datarouter/src/persistent_logging/persistent_logging_stub/stub_sysedr_handler.h"
#include "score/datarouter/src/persistent_logging/sysedr_factory.hpp"

namespace score
{
namespace platform
{
namespace internal
{

class StubSysedrFactory : public SysedrFactory<StubSysedrFactory>
{
  public:
    std::unique_ptr<ISysedrHandler> CreateConcreteSysedrHandler();
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_SRC_PERSISTENT_LOGGING_PERSISTENT_LOGGING_STUB_STUB_SYSEDR_FACTORY_H
