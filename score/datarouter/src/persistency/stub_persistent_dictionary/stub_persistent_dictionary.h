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

#ifndef SCORE_PAS_LOGGING_SRC_PERSISTENCY_STUB_PERSISTENT_DICTIONARY_STUB_PERSISTENT_DICTIONARY_H
#define SCORE_PAS_LOGGING_SRC_PERSISTENCY_STUB_PERSISTENT_DICTIONARY_STUB_PERSISTENT_DICTIONARY_H

#include "score/datarouter/src/persistency/i_persistent_dictionary.h"

namespace score
{
namespace platform
{
namespace datarouter
{

class StubPersistentDictionary final : public IPersistentDictionary
{
  public:
    std::string GetString(const std::string& key, const std::string& default_value) override;
    bool GetBool(const std::string& key, const bool default_value) override;

    void SetString(const std::string& key, const std::string& value) override;
    void SetBool(const std::string& key, const bool value) override;
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_SRC_PERSISTENCY_STUB_PERSISTENT_DICTIONARY_STUB_PERSISTENT_DICTIONARY_H
