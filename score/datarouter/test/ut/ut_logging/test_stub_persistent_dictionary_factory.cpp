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

#include "score/datarouter/src/persistency/stub_persistent_dictionary/stub_persistent_dictionary.h"
#include "score/datarouter/src/persistency/stub_persistent_dictionary/stub_persistent_dictionary_factory.h"
#include <gtest/gtest.h>

namespace score
{
namespace platform
{
namespace datarouter
{
namespace test
{

namespace
{

template <typename ConcreteDictionary>
bool IsPersistentDictionaryOfType(const std::unique_ptr<IPersistentDictionary>& persistent_dictionary) noexcept
{
    static_assert(std::is_base_of<IPersistentDictionary, ConcreteDictionary>::value,
                  "Concrete persistent_dictionary shall be derived from IPersistentDictionary base class");

    return dynamic_cast<const ConcreteDictionary*>(persistent_dictionary.get()) != nullptr;
}

TEST(PersistentDictionaryFactoryTestStub, CreatePersistentDictionaryShallReturnStubPersistentDictionary)
{
    bool no_adaptive_runtime = true;
    auto pd = StubPersistentDictionaryFactory::Create(no_adaptive_runtime);
    EXPECT_TRUE(IsPersistentDictionaryOfType<StubPersistentDictionary>(pd));
}

}  // namespace
}  // namespace test
}  // namespace datarouter
}  // namespace platform
}  // namespace score
