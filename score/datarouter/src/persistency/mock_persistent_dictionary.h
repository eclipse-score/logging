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

#ifndef SCORE_PAS_LOGGING_SRC_PERSISTENCY_MOCK_PERSISTENT_DICTIONARY_H
#define SCORE_PAS_LOGGING_SRC_PERSISTENCY_MOCK_PERSISTENT_DICTIONARY_H

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "score/datarouter/src/persistency/i_persistent_dictionary.h"

namespace score
{
namespace platform
{
namespace datarouter
{

class MockPersistentDictionary : public ::score::platform::datarouter::IPersistentDictionary
{
  public:
    MOCK_METHOD(std::string, GetString, (const std::string& key, const std::string& defaultValue), (override final));
    MOCK_METHOD(bool, GetBool, (const std::string& key, const bool defaultValue), (override final));

    MOCK_METHOD(void, SetString, (const std::string& key, const std::string& value), (override final));
    MOCK_METHOD(void, SetBool, (const std::string& key, const bool value), (override final));

  protected:
    ~MockPersistentDictionary() = default;
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_SRC_PERSISTENCY_MOCK_PERSISTENT_DICTIONARY_H
