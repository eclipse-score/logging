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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_REMOTE_DLT_RECORDER_FACTORY_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_REMOTE_DLT_RECORDER_FACTORY_H

#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/detail/log_recorder_factory.hpp"

#include "score/memory.hpp"

#include <memory>

namespace score::mw::log::detail
{

class RemoteDltRecorderFactory : public LogRecorderFactory<RemoteDltRecorderFactory>
{
  public:
    std::unique_ptr<Recorder> CreateConcreteLogRecorder(const Configuration& config,
                                                        score::cpp::pmr::memory_resource* memory_resource) noexcept;
};

}  //   namespace score::mw::log::detail

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_REMOTE_DLT_RECORDER_FACTORY_H
