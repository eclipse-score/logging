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

#ifndef SCORE_PAS_LOGGING_SRC_PERSISTENCY_STUB_PERSISTENT_DICTIONARY_STUB_PERSISTENT_DICTIONARY_FACTORY_H
#define SCORE_PAS_LOGGING_SRC_PERSISTENCY_STUB_PERSISTENT_DICTIONARY_STUB_PERSISTENT_DICTIONARY_FACTORY_H

#include "score/datarouter/src/persistency/persistent_dictionary_factory.hpp"
#include <memory>

namespace score
{
namespace platform
{
namespace datarouter
{

class StubPersistentDictionaryFactory : public PersistentDictionaryFactory<StubPersistentDictionaryFactory>
{
  public:
    static std::unique_ptr<IPersistentDictionary> CreateImpl(const bool no_adaptive_runtime);
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_SRC_PERSISTENCY_STUB_PERSISTENT_DICTIONARY_STUB_PERSISTENT_DICTIONARY_FACTORY_H
