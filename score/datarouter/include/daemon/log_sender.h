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

#ifndef LOG_SENDER_H_
#define LOG_SENDER_H_

#include "score/datarouter/include/daemon/i_log_sender.h"

namespace score
{
namespace logging
{
namespace dltserver
{
// This class is created only for test purposes, and just forward DltServer calls.
// Make no sense to test.
// LCOV_EXCL_START
class LogSender : public ILogSender
{
  public:
    LogSender() = default;
    ~LogSender() = default;
    void SendNonVerbose(const score::mw::log::config::NvMsgDescriptor& desc,
                        uint32_t tmsp,
                        const void* data,
                        size_t size,
                        DltLogChannel& c) override
    {
        c.sendNonVerbose(desc, tmsp, data, size);
    }
    void SendVerbose(uint32_t tmsp,
                     const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry,
                     DltLogChannel& c) override
    {
        c.sendVerbose(tmsp, entry);
    }
    void SendFTVerbose(score::cpp::span<const std::uint8_t> data,
                       mw::log::LogLevel loglevel,
                       dltid_t appId,
                       dltid_t ctxId,
                       uint8_t nor,
                       uint32_t tmsp,
                       DltLogChannel& c) override
    {
        c.sendFTVerbose(data, loglevel, appId, ctxId, nor, tmsp);
    }
};
// LCOV_EXCL_STOP
}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // LOG_SENDER_H_
