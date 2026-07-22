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

#ifndef SCORE_DATAROUTER_SRC_FILE_TRANSFER_FILE_TRANSFER_IMPL_FILETRANSFER_STREAM_H
#define SCORE_DATAROUTER_SRC_FILE_TRANSFER_FILE_TRANSFER_IMPL_FILETRANSFER_STREAM_H

#include "daemon/dltserver_common.h"
#include "daemon/udp_stream_output.h"
#include "logparser/logparser.h"

#include "score/mw/log/log_level.h"
#include "score/datarouter/dlt_filetransfer_trigger_lib/filetransfer_message_trace.h"

#include <score/jthread.hpp>

#include <atomic>
#include <queue>
#include <shared_mutex>
#include <string>

using namespace score::platform::internal;

namespace score
{
namespace logging
{
namespace dltserver
{

class FileTransferStreamHandler : public LogParser::TypeHandler
{
  public:
    class IOutput
    {
      public:
        virtual void SendFtVerbose(score::cpp::span<const std::uint8_t> data,
                                   mw::log::LogLevel loglevel,
                                   DltidT app_id,
                                   DltidT ctx_id,
                                   uint8_t nor,
                                   uint32_t time_tmsp) = 0;
        virtual bool IsOutputEnabled() const noexcept = 0;

        virtual ~IOutput() = default;
    };
    explicit FileTransferStreamHandler(IOutput& output);
    ~FileTransferStreamHandler() noexcept;
    virtual void Handle(TimestampT timestamp, const char* data, BufsizeT size) override;

  private:
    using DltDurationT = std::chrono::duration<uint32_t, std::ratio<1, 10000>>;
    void ProcessFileTransfer();
    bool ReadFileHeaderInfo(const std::string& filename);
    bool LogFileHeader();
    void LogFileData();
    void LogFileEnd();
    void LogFileError(int16_t errorcode, std::string error_msg = "");
    score::cpp::jthread filetransfer_thread_;
    std::atomic_bool exit_requested_;
    std::shared_timed_mutex filetransfer_mutex_;
    std::queue<::score::logging::FileTransferEntry> container_;
    std::string readfile_, creationdate_;
    DltidT appid_;
    DltidT ctxid_;
    std::uint32_t serialno_, fsize_, packagecount_;
    IOutput& output_;
};

}  // namespace dltserver
}  // namespace logging
}  // namespace score

#endif  // SCORE_DATAROUTER_SRC_FILE_TRANSFER_FILE_TRANSFER_IMPL_FILETRANSFER_STREAM_H
