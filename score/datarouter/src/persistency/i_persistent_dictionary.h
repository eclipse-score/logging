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

#ifndef SCORE_PAS_LOGGING_SRC_PERSISTENCY_I_PERSISTENT_DICTIONARY_H
#define SCORE_PAS_LOGGING_SRC_PERSISTENCY_I_PERSISTENT_DICTIONARY_H

#include <string>

namespace score
{
namespace platform
{
namespace datarouter
{

// Suppress "AUTOSAR C++14 M3-2-3" rule finding. This rule states: "A type, object or function that is used in multiple
// translation units shall be declared in one and only one file.".
// This is false positive. IPersistentLogRequestor is declared only once.
// coverity[autosar_cpp14_m3_2_3_violation : FALSE]
class IPersistentDictionary
{
  public:
    // Public interface shall be thread-safe.

    virtual std::string GetString(const std::string& key, const std::string& default_value) = 0;
    virtual bool GetBool(const std::string& key, const bool default_value) = 0;

    virtual void SetString(const std::string& key, const std::string& value) = 0;
    virtual void SetBool(const std::string& key, const bool value) = 0;

    virtual ~IPersistentDictionary() = default;
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_SRC_PERSISTENCY_I_PERSISTENT_DICTIONARY_H
