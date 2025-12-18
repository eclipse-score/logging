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

#ifndef DLTSERVER_COMMON_H
#define DLTSERVER_COMMON_H

#include "dlt/dltid.h"
#include "logparser/logparser.h"

namespace score
{
namespace logging
{
namespace dltserver
{

using score::platform::bufsize_t;
using score::platform::dltid_t;
using score::platform::timestamp_t;
using score::platform::TypeInfo;
using score::platform::internal::LogParser;

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // DLTSERVER_COMMON_H
