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

#include "score/mw/log/detail/data_router/data_router_message_client_backend.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

MsgClientBackend::MsgClientBackend(SharedMemoryWriter& shared_memory_writer,
                                   const std::string& writer_file_name,
                                   std::unique_ptr<MessagePassingFactory> message_passing_factory,
                                   const bool use_dynamic_datarouter_ids)
    : shared_memory_writer_{shared_memory_writer},
      writer_file_name_{writer_file_name},
      message_passing_factory_{std::move(message_passing_factory)},
      use_dynamic_datarouter_ids_{use_dynamic_datarouter_ids}
{
}

SharedMemoryWriter& MsgClientBackend::GetShMemWriter() const noexcept
{
    return shared_memory_writer_;
}

const std::string& MsgClientBackend::GetWriterFilename() const noexcept
{
    return writer_file_name_;
}

std::unique_ptr<MessagePassingFactory>& MsgClientBackend::GetMsgPassingFactory() noexcept
{
    // Returning address of non-static class member is justified by design
    // coverity[autosar_cpp14_a9_3_1_violation]
    return message_passing_factory_;
}

bool MsgClientBackend::IsUsingDynamicDatarouterIDs() const noexcept
{
    return use_dynamic_datarouter_ids_;
}

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
