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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_BACKEND_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_BACKEND_H

#include "score/mw/log/detail/data_router/message_passing_factory.h"
#include "score/mw/log/detail/data_router/shared_memory/shared_memory_writer.h"
#include <iostream>
#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

class MsgClientBackend
{
  public:
    MsgClientBackend(SharedMemoryWriter& shared_memory_writer,
                     const std::string& writer_file_name,
                     std::unique_ptr<MessagePassingFactory> message_passing_factory,
                     const bool use_dynamic_datarouter_ids);

    SharedMemoryWriter& GetShMemWriter() const noexcept;
    const std::string& GetWriterFilename() const noexcept;
    std::unique_ptr<MessagePassingFactory>& GetMsgPassingFactory() noexcept;
    bool IsUsingDynamicDatarouterIDs() const noexcept;

  private:
    SharedMemoryWriter& shared_memory_writer_;
    std::string writer_file_name_;
    std::unique_ptr<MessagePassingFactory> message_passing_factory_;
    bool use_dynamic_datarouter_ids_;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_BACKEND_H
