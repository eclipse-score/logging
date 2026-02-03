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

#ifndef SCORE_DATAROUTER_INCLUDE_UNIX_DOMAIN_UNIX_DOMAIN_CLIENT_H
#define SCORE_DATAROUTER_INCLUDE_UNIX_DOMAIN_UNIX_DOMAIN_CLIENT_H

#include "unix_domain/unix_domain_common.h"

#include <unistd.h>

#include <score/string_view.hpp>

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include <score/string_view.hpp>

#include "score/os/utils/signal.h"
#include "score/os/utils/signal_impl.h"

namespace score
{
namespace platform
{
namespace internal
{

class UnixDomainClient
{
  public:
    using Callback = std::function<void(void)>;
    using TickCallback = std::function<bool(void)>;
    using FdCallback = std::function<void(int)>;
    using RequestCallback = std::function<void(const std::string&)>;
    UnixDomainClient(UnixDomainSockAddr addr,
                     Callback on_connect,
                     Callback on_disconnect,
                     FdCallback on_fd = FdCallback(),
                     TickCallback on_tick = TickCallback(),
                     RequestCallback on_request = RequestCallback(),
                     std::unique_ptr<score::os::Signal> signal = std::make_unique<score::os::SignalImpl>())
        : addr_(addr),
          exit_(false),
          commands_mutex_{},
          commands_{},
          client_thread_{},
          new_socket_retry_{false},
          fd_{-1},
          on_connect_(on_connect),
          on_disconnect_(on_disconnect),
          on_fd_(on_fd),
          on_tick_(on_tick),
          on_request_(on_request),
          signal_(std::move(signal))
    {
    }

    ~UnixDomainClient()
    {
        exit_ = true;
        if (client_thread_.joinable())
        {
            client_thread_.join();
        }
    }

    void SendResponse(const std::string& response)
    {
        SendSocketMessage(fd_, response);
    }

    void Ping();

  private:
    void ClientRoutine();
    void UpdateThreadNameLogger();

    UnixDomainSockAddr addr_;
    std::atomic<bool> exit_;
    std::mutex commands_mutex_;
    std::queue<std::string> commands_;
    std::thread client_thread_;
    bool new_socket_retry_;

    // TODO: temporary workaround, remove
    std::atomic<std::int32_t> fd_;

    Callback on_connect_;
    Callback on_disconnect_;
    FdCallback on_fd_;
    TickCallback on_tick_;
    RequestCallback on_request_;

    std::unique_ptr<score::os::Signal> signal_;
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // SCORE_DATAROUTER_INCLUDE_UNIX_DOMAIN_UNIX_DOMAIN_CLIENT_H
