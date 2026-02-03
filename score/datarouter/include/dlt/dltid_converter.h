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

#ifndef SCORE_DATAROUTER_INCLUDE_DLT_DLTID_CONVERTER_H
#define SCORE_DATAROUTER_INCLUDE_DLT_DLTID_CONVERTER_H

#include "score/mw/log/detail/logging_identifier.h"
#include "score/datarouter/include/dlt/dltid.h"

namespace score
{
namespace platform
{

DltidT ConvertToDltId(const score::mw::log::detail::LoggingIdentifier& logging_identifier);

}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_DLT_DLTID_CONVERTER_H
