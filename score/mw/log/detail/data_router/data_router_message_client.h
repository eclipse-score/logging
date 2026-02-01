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

#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_H

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief The class is responsible for communicating with Datarouter
/// and synchronizing the access to the ring buffer.
/// The public interface is not thread-safe.
class DatarouterMessageClient
{
  public:
    DatarouterMessageClient() = default;
    virtual ~DatarouterMessageClient();
    DatarouterMessageClient(DatarouterMessageClient&&) = delete;
    DatarouterMessageClient(const DatarouterMessageClient&) = delete;
    DatarouterMessageClient& operator=(DatarouterMessageClient&&) = delete;
    DatarouterMessageClient& operator=(const DatarouterMessageClient&) = delete;

    /// \brief Starts the connection to Datarouter in a background thread.
    /// Must only be called once.
    virtual void Run() = 0;

    /// \brief Blocks until the communication to Datarouter is closed.
    virtual void Shutdown() = 0;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_DATA_ROUTER_MESSAGE_CLIENT_H
