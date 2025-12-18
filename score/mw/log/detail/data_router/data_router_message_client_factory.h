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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_FACTORY_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_FACTORY_H

#include "score/mw/log/detail/data_router/data_router_message_client.h"
#include "score/mw/log/detail/data_router/shared_memory/writer_factory.h"

#include <memory>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief The interface encapsulates the creation routine of a DatarouterMessageClient.
/// The factory shall only support single time usage.
/// The interface is not thread safe.
class DatarouterMessageClientFactory
{
  public:
    DatarouterMessageClientFactory() = default;
    virtual ~DatarouterMessageClientFactory();
    DatarouterMessageClientFactory(DatarouterMessageClientFactory&&) = delete;
    DatarouterMessageClientFactory(const DatarouterMessageClientFactory&) = delete;
    DatarouterMessageClientFactory& operator=(DatarouterMessageClientFactory&&) = delete;
    DatarouterMessageClientFactory& operator=(const DatarouterMessageClientFactory&) = delete;

    /// \brief Creates a new message client instance.
    /// \pre This method must be called maximum one time.
    virtual std::unique_ptr<DatarouterMessageClient> CreateOnce(const std::string& identifer,
                                                                const std::string& mwsr_file_name) = 0;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_CLIENT_H
