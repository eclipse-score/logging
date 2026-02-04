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

#include "stub_persistent_dictionary.h"

namespace score
{
namespace platform
{
namespace datarouter
{

std::string StubPersistentDictionary::getString(const std::string& /*key*/, const std::string& defaultValue)
{
    return defaultValue;
}

bool StubPersistentDictionary::getBool(const std::string& /*key*/, const bool defaultValue)
{
    return defaultValue;
}
// LCOV_EXCL_START : will suppress this function since there nothing to assert through UT as its void empty function
void StubPersistentDictionary::setString(const std::string& /*key*/, const std::string& /*value*/) {}
// LCOV_EXCL_STOP

// LCOV_EXCL_START : will suppress this function since there nothing to assert through UT as its void empty function
void StubPersistentDictionary::setBool(const std::string& /*key*/, const bool /*value*/) {}
// LCOV_EXCL_STOP

}  // namespace datarouter
}  // namespace platform
}  // namespace score
