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

#include "score/mw/log/detail/data_router/remote_dlt_recorder_factory.h"

#include "score/mw/log/detail/data_router/data_router_backend.h"
#include "score/mw/log/detail/data_router/data_router_message_client_factory_impl.h"
#include "score/mw/log/detail/data_router/data_router_recorder.h"
#include "score/mw/log/detail/data_router/message_passing_factory_impl.h"

#include "score/os/utils/signal_impl.h"

namespace score::mw::log::detail
{

std::unique_ptr<Recorder> RemoteDltRecorderFactory::CreateConcreteLogRecorder(
    const Configuration& config,
    score::cpp::pmr::memory_resource* memory_resource) noexcept
{
    auto message_client_factory = std::make_unique<DatarouterMessageClientFactoryImpl>(
        config,
        std::make_unique<MessagePassingFactoryImpl>(),
        MsgClientUtils{score::os::Unistd::Default(memory_resource),
                       score::os::Pthread::Default(memory_resource),
                       score::cpp::pmr::make_unique<score::os::SignalImpl>(memory_resource)});
    WriterFactory::OsalInstances writer_factory_osal = {score::os::Fcntl::Default(memory_resource),
                                                        score::os::Unistd::Default(memory_resource),
                                                        score::os::Mman::Default(memory_resource),
                                                        score::os::Stat::Default(memory_resource),
                                                        score::os::Stdlib::Default(memory_resource)};

    //  Although std::make_unique may throw (e.g., on memory allocation failure), this function is marked noexcept
    //  because our design assumes that the provided memory_resource is nothrow, and any allocation failure is
    //  considered unrecoverable (triggering std::terminate).
    // coverity[autosar_cpp14_a15_4_2_violation]
    return std::make_unique<DataRouterRecorder>(
        std::make_unique<DataRouterBackend>(config.GetNumberOfSlots(),
                                            LogRecord{config.GetSlotSizeInBytes()},
                                            *message_client_factory,
                                            config,
                                            WriterFactory{std::move(writer_factory_osal)}),
        config);
}

}  //   namespace score::mw::log::detail
