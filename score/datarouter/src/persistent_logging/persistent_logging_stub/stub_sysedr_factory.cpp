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

#include "score/datarouter/src/persistent_logging/persistent_logging_stub/stub_sysedr_factory.h"
#include "score/mw/log/logging.h"

namespace score
{
namespace platform
{
namespace internal
{

std::unique_ptr<ISysedrHandler> StubSysedrFactory::CreateConcreteSysedrHandler()
{
    score::mw::log::LogError() << "persistent_logging disabled, no persistent logging actions performed";
    return std::make_unique<StubSysedrHandler>();
}

}  // namespace internal
}  // namespace platform
}  // namespace score
