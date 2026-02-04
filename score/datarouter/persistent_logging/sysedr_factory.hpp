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

#ifndef SCORE_PAS_LOGGING_INCLUDE_SYSEDR_SYSEDR_FACTORY_H
#define SCORE_PAS_LOGGING_INCLUDE_SYSEDR_SYSEDR_FACTORY_H

#include "score/datarouter/persistent_logging/isysedr_handler.h"
#include <memory>

namespace score
{
namespace platform
{
namespace internal
{
template <typename DerivedSysedrHandler>
class SysedrFactory
{
  public:
    std::unique_ptr<ISysedrHandler> CreateSysedrHandler()
    {
        return static_cast<DerivedSysedrHandler*>(this)->CreateConcreteSysedrHandler();
    }

  private:
    SysedrFactory() = default;
    ~SysedrFactory() = default;
    SysedrFactory(SysedrFactory&) = delete;
    SysedrFactory(SysedrFactory&&) = delete;
    SysedrFactory& operator=(SysedrFactory&) = delete;
    SysedrFactory& operator=(SysedrFactory&&) = delete;

    /*
    Deviation from rule: A11-3-1
    - "Friend declarations shall not be used."
    Justification:
    - friend class and private constructor used here to prevent wrong inheritance
    */
    // coverity[autosar_cpp14_a11_3_1_violation]
    friend DerivedSysedrHandler;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_INCLUDE_SYSEDR_SYSEDR_FACTORY_H
