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

#ifndef LOGGING_APPLICATIONS_DATAROUTER_APP_H_
#define LOGGING_APPLICATIONS_DATAROUTER_APP_H_

#include <atomic>

namespace score
{
namespace logging
{
namespace datarouter
{

extern void datarouter_app_init();
extern void datarouter_app_run(const std::atomic_bool& exit_requested);
extern void datarouter_app_shutdown();

}  // namespace datarouter
}  // namespace logging
}  // namespace score

#endif  // LOGGING_APPLICATIONS_DATAROUTER_APP_H_
