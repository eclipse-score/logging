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

#ifndef PLOGFILTERDESC_H_
#define PLOGFILTERDESC_H_

#include "score/mw/log/detail/log_entry.h"

namespace score
{
namespace platform
{
namespace internal
{

struct plogfilterdesc
{
    score::mw::log::detail::LoggingIdentifier appid_;
    score::mw::log::detail::LoggingIdentifier ctxid_;
    uint8_t logLevel_{};
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // PLOGFILTERDESC_H_
