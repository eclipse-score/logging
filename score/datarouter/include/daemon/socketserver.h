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

#ifndef LOGGING_SOCKETSERVER_H
#define LOGGING_SOCKETSERVER_H

#include <atomic>
#include <iostream>

namespace score
{
namespace platform
{
namespace datarouter
{

class SocketServer
{
  public:
    static void run(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime)
    {
        static SocketServer server;
        server.doWork(exit_requested, no_adaptive_runtime);
    }

  private:
    void doWork(const std::atomic_bool& exit_requested, const bool no_adaptive_runtime);
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // LOGGING_SOCKETSERVER_H
