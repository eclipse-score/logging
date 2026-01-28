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

#ifndef CONFIG_SESSION_FACTORY_H
#define CONFIG_SESSION_FACTORY_H

#include "i_session.h"
#include <functional>
#include <memory>

namespace score
{
namespace logging
{
namespace daemon
{

template <typename DerivedFactory>
class ConfigSessionFactory
{
  public:
    template <typename Handle, typename Handler>
    std::unique_ptr<score::logging::ISession> CreateConfigSession(Handle&& handle, Handler&& handler)
    {
        return static_cast<DerivedFactory&>(*this).CreateConcreteSession(std::forward<Handle>(handle),
                                                                         std::forward<Handler>(handler));
    }

  private:
    // prevent wrong inheritance
    ConfigSessionFactory() = default;
    ~ConfigSessionFactory() = default;
    ConfigSessionFactory(ConfigSessionFactory&) = delete;
    ConfigSessionFactory(ConfigSessionFactory&&) = delete;
    ConfigSessionFactory& operator=(ConfigSessionFactory&) = delete;
    ConfigSessionFactory& operator=(ConfigSessionFactory&&) = delete;

    /*
    Deviation from rule: A11-3-1
    - "Friend declarations shall not be used."
    Justification:
    - friend class and private constructor used here to prevent wrong inheritance
    */
    // coverity[autosar_cpp14_a11_3_1_violation]
    friend DerivedFactory;
};

}  // namespace daemon
}  // namespace logging
}  // namespace score

#endif  // CONFIG_SESSION_FACTORY_H
