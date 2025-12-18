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

#ifndef UNIX_DOMAIN_CLIENT_H_
#define UNIX_DOMAIN_CLIENT_H_

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
                     Callback onConnect,
                     Callback onDisconnect,
                     FdCallback onFd = FdCallback(),
                     TickCallback onTick = TickCallback(),
                     RequestCallback onRequest = RequestCallback(),
                     std::unique_ptr<score::os::Signal> signal = std::make_unique<score::os::SignalImpl>())
        : addr_(addr),
          exit_(false),
          commands_mutex_{},
          commands_{},
          client_thread{},
          new_socket_retry_{false},
          fd_{-1},
          on_connect_(onConnect),
          on_disconnect_(onDisconnect),
          on_fd_(onFd),
          on_tick_(onTick),
          on_request_(onRequest),
          signal_(std::move(signal))
    {
    }

    ~UnixDomainClient()
    {
        exit_ = true;
        if (client_thread.joinable())
        {
            client_thread.join();
        }
    }

    void send_response(const std::string& response)
    {
        send_socket_message(fd_, response);
    }

    void ping();

  private:
    void client_routine();
    void update_thread_name_logger();

    UnixDomainSockAddr addr_;
    std::atomic<bool> exit_;
    std::mutex commands_mutex_;
    std::queue<std::string> commands_;
    std::thread client_thread;
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

#endif  // UNIX_DOMAIN_CLIENT_H_
