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

#include "stub_persistent_dictionary_factory.h"
#include "stub_persistent_dictionary.h"

namespace score
{
namespace platform
{
namespace datarouter
{

std::unique_ptr<IPersistentDictionary> StubPersistentDictionaryFactory::CreateImpl(
    [[maybe_unused]] const bool no_adaptive_runtime)
{
    return std::make_unique<StubPersistentDictionary>();
}

}  // namespace datarouter
}  // namespace platform
}  // namespace score
