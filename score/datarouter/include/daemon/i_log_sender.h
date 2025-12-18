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

#ifndef I_LOG_SENDER_H_
#define I_LOG_SENDER_H_

#include "score/mw/log/log_level.h"
#include "score/datarouter/include/daemon/dlt_log_channel.h"

namespace score
{
namespace logging
{
namespace dltserver
{

// This is the interface with no functionality.
// Make no sense to test.
// LCOV_EXCL_START
class ILogSender
{
  public:
    virtual ~ILogSender() = default;
    virtual void SendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                                uint32_t tmsp,
                                const void* data,
                                size_t size,
                                DltLogChannel& c) = 0;
    virtual void SendVerbose(
        uint32_t tmsp,
        const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry,
        DltLogChannel& c) = 0;
    virtual void SendFTVerbose(score::cpp::span<const std::uint8_t> data,
                               mw::log::LogLevel loglevel,
                               dltid_t appId,
                               dltid_t ctxId,
                               uint8_t nor,
                               uint32_t tmsp,
                               DltLogChannel& c) = 0;
};
// LCOV_EXCL_STOP

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // I_LOG_SENDER_H_
