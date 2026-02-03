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

#ifndef SCORE_PAS_LOGGING_SRC_PERSISTENT_DICTIONARY_FACTORY_HPP
#define SCORE_PAS_LOGGING_SRC_PERSISTENT_DICTIONARY_FACTORY_HPP

#include "i_persistent_dictionary.h"
#include <memory>

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
template <typename PersistentDictionary>
class PersistentDictionaryFactory
{
  public:
    static std::unique_ptr<IPersistentDictionary> Create(const bool no_adaptive_runtime)
    {
        return PersistentDictionary::CreateImpl(no_adaptive_runtime);
    }

  private:
    // prevent wrong inheritance
    PersistentDictionaryFactory() = default;
    ~PersistentDictionaryFactory() = default;
    PersistentDictionaryFactory(PersistentDictionaryFactory&) = delete;
    PersistentDictionaryFactory(PersistentDictionaryFactory&&) = delete;
    PersistentDictionaryFactory& operator=(PersistentDictionaryFactory&) = delete;
    PersistentDictionaryFactory& operator=(PersistentDictionaryFactory&&) = delete;

    /*
    Deviation from rule: A11-3-1
    - "Friend declarations shall not be used."
    Justification:
    - friend class and private constructor used here to prevent wrong inheritance
    */
    // coverity[autosar_cpp14_a11_3_1_violation]
    friend PersistentDictionary;
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_SRC_PERSISTENT_DICTIONARY_FACTORY_HPP
