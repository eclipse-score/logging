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

#include "unix_domain/unix_domain_client.h"
#include "score/os/pthread.h"
#include "score/os/utils/signal_impl.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <thread>

namespace score
{
namespace platform
{
namespace internal
{
namespace
{

#if defined(__QNXNTO__)
score::cpp::optional<SharedMemoryFileHandle> OpenReceivedSharedMemoryFileHandle(const score::cpp::span<std::uint8_t> data)
{
    score::cpp::optional<SharedMemoryFileHandle> result = score::cpp::nullopt;
    using SharedMemoryFileHandleInternal = shm_handle_t;
    if (data.size() != sizeof(SharedMemoryFileHandleInternal))
    {
        result = score::cpp::nullopt;
        return result;
    }
    SharedMemoryFileHandleInternal shared_memory_file_handle{};
    //  the type is used to encode number of elements and thus shall not have negative value:
    const auto data_size = static_cast<std::size_t>(data.size());
    // NOLINTNEXTLINE(score-banned-function) memcpy is needed to set shm_handle_t from received message
    score::cpp::ignore = std::memcpy(static_cast<void*>(&shared_memory_file_handle), data.data(), data_size);
    // Suppressed here because usage of this OSAL method is on banned list
    // NOLINTNEXTLINE(score-banned-function) see above
    std::int32_t open_result_fd = shm_open_handle(shared_memory_file_handle, O_RDWR);
    if (open_result_fd == -1)
    {
        std::cerr << "Try to open received shared memory file failed on client side" << std::endl;
        result = score::cpp::nullopt;
    }
    else
    {
        result = open_result_fd;
    }
    return result;
}
#endif  //  defined(__QNXNTO__)
}  //  namespace

void UnixDomainClient::client_routine()
{
    SetupSignals(signal_);

    std::uniform_int_distribution<int32_t> reconnectMsRange(75, 125);
    std::mt19937 gen{std::random_device{}()};
    // random per thread instance, constant for a thread instance, initialized early
    const auto reconnectDelay = std::chrono::milliseconds(reconnectMsRange(gen));

    while (false == exit_.load())
    {
        new_socket_retry_ = false;
        // Suppressed here as it is safely used, and it is among safety headers.
        // NOLINTNEXTLINE(score-banned-function) see comment above
        std::int32_t fd = socket(AF_UNIX, SOCK_STREAM, 0);

        using namespace std::chrono_literals;
        if (fd == -1)
        {
            std::this_thread::sleep_for(100ms);
            /*
            Deviation from Rule M6-6-3:
            - The continue statement shall only be used within a well-formed for loop
            Justification:
            - used for while loop
            */
            // coverity[autosar_cpp14_m6_6_3_violation]
            continue;
        }

        while (false == exit_.load())
        {
            auto connectRetryDelay = 100ms;
            std::int32_t ret =
                connect(fd, static_cast<const sockaddr*>(static_cast<const void*>(&addr_)), sizeof(sockaddr_un));
            if (ret == -1)
            {
                // TODO: ENOENT was added to allow applications to run during QNX transition
                // see: TicketOld-68843
                /*
                Deviation from Rule M19-3-1:
                - The error indicator errno shall not be used.
                Justification:
                - required for error handling
                */
                // coverity[autosar_cpp14_m19_3_1_violation]
                if (errno == ECONNREFUSED || errno == EAGAIN || errno == ENOENT)
                {
                    std::this_thread::sleep_for(connectRetryDelay);
                    connectRetryDelay *= 2;
                    if (connectRetryDelay > 5000ms)
                    {
                        connectRetryDelay = 5000ms;
                    }
                    /*
                    Deviation from Rule M6-6-3:
                    - The continue statement shall only be used within a well-formed for loop
                    Justification:
                    - used for while loop
                    */
                    // coverity[autosar_cpp14_m6_6_3_violation]
                    continue;
                }
                new_socket_retry_ = true;
                std::cerr << "new_socket_retry = true";
            }
            break;
        }
        if (true == exit_.load() || new_socket_retry_)
        {
            // Suppressed here as it is safely used, and it is among safety headers.
            // NOLINTNEXTLINE(score-banned-function) see comment above
            close(fd);
            fd = -1;
            break;
        }
        fd_.store(fd);
        on_connect_();

        struct timeval tout{};
        tout.tv_sec = 0;
        tout.tv_usec = 100000;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout)) < 0)
        {
            std::perror("setsockopt SO_RCVTIMEO");
            // NOLINTNEXTLINE(score-banned-function): Suppressed here because of error handling
            std::exit(EXIT_FAILURE);
        }
        bool command_in_transit = false;
        while (false == exit_.load())
        {
            if (on_tick_)
            {
                while (on_tick_())
                {
                }
            }
            std::string out_string;
            if (!command_in_transit)
            {
                std::lock_guard<std::mutex> lock(commands_mutex_);
                if (!commands_.empty())
                {
                    out_string = commands_.front();
                    if (out_string.empty())
                    {
                        // std::cout << "empty command!" << std::endl;
                        commands_.pop();
                    }
                    else
                    {
                        send_socket_message(fd, out_string);
                        command_in_transit = true;
                    }
                }
            }

            score::cpp::optional<std::int32_t> pid_in;
            score::cpp::optional<std::int32_t> result_fd;
#if defined(__QNXNTO__)
            const auto response = recv_socket_message(fd, result_fd, pid_in, OpenReceivedSharedMemoryFileHandle);
#else
            const auto response = recv_socket_message(fd, result_fd, pid_in);
#endif  //  defined(__QNXNTO__)

            if (false == response.has_value())
            {
                std::this_thread::sleep_for(reconnectDelay);
                break;
            }

            if (result_fd.has_value())
            {
                if (on_fd_)
                {
                    on_fd_(result_fd.value());
                }
                else
                {
                    // Suppressed here as it is safely used, and it is among safety headers.
                    // NOLINTNEXTLINE(score-banned-function) see comment above
                    close(result_fd.value());
                }
            }

            if (!response.value().empty())
            {
                std::unique_lock<std::mutex> lock(commands_mutex_);
                if (command_in_transit && commands_.front() == response.value())
                {
                    // command arrived to the server
                    commands_.pop();
                    command_in_transit = false;
                }
                else
                {
                    lock.unlock();
                    if (on_request_)
                    {
                        on_request_(response.value());
                    }
                }
            }
        }
        if (on_tick_)
        {
            while (on_tick_())
            {
            }
        }
        fd_.store(-1);
        on_disconnect_();
        // Suppressed here as it is safely used, and it is among safety headers.
        // NOLINTNEXTLINE(score-banned-function) see comment above
        close(fd);
    }
}

void UnixDomainClient::ping()
{
    if (true != exit_.load() && fd_.load() != -1)
    {
        send_socket_message(fd_.load(), std::string());
    }
}

void UnixDomainClient::update_thread_name_logger()
{
    auto ret = score::os::Pthread::instance().setname_np(client_thread.native_handle(), "logger");
    if (!ret.has_value())
    {
        auto errstr = ret.error().ToString();
        std::fprintf(stderr, "pthread_setname_np: %s\n", errstr.c_str());
    }
}

}  // namespace internal
}  // namespace platform
}  // namespace score
