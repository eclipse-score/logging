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

#ifndef DLTID_CONVERTER_H_
#define DLTID_CONVERTER_H_

#include "score/mw/log/detail/logging_identifier.h"
#include "score/datarouter/include/dlt/dltid.h"

namespace score
{
namespace platform
{

dltid_t convertToDltId(const score::mw::log::detail::LoggingIdentifier& logging_identifier);

}  // namespace platform
}  // namespace score

#endif  // DLTID_CONVERTER_H_
