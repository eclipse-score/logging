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

#include "unix_domain/unix_domain_common.h"
#include "score/os/errno.h"
#include "score/os/socket.h"
#include "score/os/utils/signal.h"
#include <unistd.h>
#include <cstring>

#include <array>
#include <atomic>
#include <iostream>

#include <fcntl.h>
#include <sys/mman.h>

namespace score
{
namespace platform
{
namespace internal
{

#if defined(__QNXNTO__)
#define USE_SECURE_FILE_HANDLE_IPC 1
#else
#define USE_SECURE_FILE_HANDLE_IPC 0
#endif

namespace
{

constexpr size_t kSocketCmsgSpace{24};

#ifdef __linux__
// CMSG_SPACE() is constexpr only on Linux.
static_assert(kSocketCmsgSpace == CMSG_SPACE(sizeof(int) * 1U), "Invalid constant value for kSocketCmsgSpace");
#endif  // __linux__

}  // namespace

void cmsg_len_suppress_warnings(struct cmsghdr* cmsg_, size_t num_fds_)
{
#ifdef __QNX__
// NOLINTBEGIN(score-banned-preprocessor-directives) : required due to compiler warning for qnx
/*
Deviation from Rule A16-7-1:
- The #pragma directive shall not be used
Justification:
- required due to compiler warning for qnx
*/
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic push
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic ignored "-Wsign-conversion"
    cmsg_->cmsg_len = static_cast<socklen_t>(CMSG_LEN(sizeof(std::int32_t) * num_fds_));
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic pop
// NOLINTEND(score-banned-preprocessor-directives)
#else
    cmsg_->cmsg_len = CMSG_LEN(sizeof(std::int32_t) * num_fds_);
#endif
}

int32_t* cmsg_data_suppress_warning(struct cmsghdr* cmsg_)
{
    int32_t* fdptr_ = nullptr;
#ifdef __QNX__
// NOLINTBEGIN(score-banned-preprocessor-directives) : required due to compiler warning for qnx
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic push
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic ignored "-Wsign-conversion"
    /*
    Deviation from Rule M5-0-15:
    - Array indexing shall be the only form of pointer arithmetic
    Deviation from Rule M5-2-8:
    - An object with integer type or pointer to void type shall not be converted to an object with pointer type
    Justification:
    - socket macro required
    - required for c-style struct member assignment
    */
    // coverity[autosar_cpp14_m5_0_15_violation]
    // coverity[autosar_cpp14_m5_2_8_violation]
    fdptr_ = static_cast<std::int32_t*>(static_cast<void*>(CMSG_DATA(cmsg_)));
// coverity[autosar_cpp14_a16_7_1_violation] see above
#pragma GCC diagnostic pop
// NOLINTEND(score-banned-preprocessor-directives)
#else
    // coverity[autosar_cpp14_m5_0_15_violation] see above
    // coverity[autosar_cpp14_m5_2_8_violation] see above
    fdptr_ = static_cast<std::int32_t*>(static_cast<void*>(CMSG_DATA(cmsg_)));
#endif
    return fdptr_;
}

UnixDomainSockAddr::UnixDomainSockAddr(const std::string& path, bool isAbstract) : addr_()
{
    addr_.sun_family = static_cast<sa_family_t>(AF_UNIX);
    size_t len = std::min(sizeof(addr_.sun_path) - 1U - (isAbstract ? 1U : 0U), path.length());
    auto addr_iter = std::begin(addr_.sun_path);
    std::advance(addr_iter, static_cast<int32_t>(isAbstract));
    std::copy_n(path.begin(), len, addr_iter);
}

void SendAncillaryDataOverSocket(int connection_file_descriptor, score::cpp::span<std::uint8_t> data)
{
    int pid = getpid();
    SocketMessangerHeader messanger_header{};
    messanger_header.type = MessageType::kSharedMemoryFileHandle;
    messanger_header.len = static_cast<uint16_t>(data.size());
    messanger_header.pid = pid;

    struct msghdr msg{};

    constexpr auto kVectorCount = 2UL;
    std::array<iovec, kVectorCount> io{};
    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) iovec is unchangeble, It's (POSIX standard)
    io[0UL].iov_base = &messanger_header;
    io[0UL].iov_len = sizeof(messanger_header);
    /*
    Deviation from Rule M5-2-8:
    - An object with integer type or pointer to void type shall not be converted to an object with pointer type
    Justification:
    - required for c-style struct member assignment
    */
    // coverity[autosar_cpp14_m5_2_8_violation]
    io[1UL].iov_base = static_cast<char*>(static_cast<void*>(data.data()));
    io[1UL].iov_len = static_cast<std::size_t>(data.size());
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)

    msg.msg_iov = io.data();
    msg.msg_iovlen = kVectorCount;

    msg.msg_control = nullptr;
    msg.msg_controllen = 0UL;

    auto ret =
        score::os::Socket::instance().sendmsg(connection_file_descriptor, &msg, score::os::Socket::MessageFlag::kNone);

    if ((ret.has_value() == false) || (ret.value() == -1))
    {
        if (ret.error() == score::os::Error::Code::kResourceTemporarilyUnavailable)
        {
            static std::atomic_bool eagain_reported{false};
            if (true == eagain_reported.load())
            {
                return;
            }
            eagain_reported = true;
            std::perror("sendmsg");
        }
        else
        {
            std::cerr << "sendmsg: Error reported with errno: " << ret.error().ToString() << std::endl;
        }
    }
}

/* Send message without file descriptor option *
 * One of the function is to pass a handle to shared memory file */
void send_socket_message(std::int32_t connection_file_descriptor,
                         score::cpp::string_view message,
                         score::cpp::optional<SharedMemoryFileHandle> file_handle)
{
    // TODO: currently checking viability. Redo in a safe way

    if (file_handle.has_value() && file_handle.value() < 0)
    {
        file_handle = score::cpp::nullopt;
    }

    int pid = getpid();
    SocketMessangerHeader messanger_header{};
    messanger_header.type = MessageType::kDefault;
    messanger_header.len = static_cast<uint16_t>(message.size() + 1U);
    messanger_header.pid = pid;

    struct msghdr msg{};

    constexpr auto kVectorCount = 2UL;
    std::array<iovec, kVectorCount> io{};
    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) iovec is unchangeble, It's (POSIX standard)
    io.at(0UL).iov_base = &messanger_header;
    io.at(0).iov_len = sizeof(messanger_header);
    /*
    Deviation from Rule A5-2-3:
    - A cast shall not remove any const or volatile qualification from the type of a pointer or reference
    Justification:
    - const_cast is necessary to remove const qualifier in order to set iov_base.
    */
    // coverity[autosar_cpp14_a5_2_3_violation]
    io.at(1UL).iov_base = const_cast<char*>(message.data());
    io.at(1UL).iov_len = message.size() + 1U;
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)

    msg.msg_iov = &io.at(0);
    msg.msg_iovlen = kVectorCount;

    union
    {
        // It is unsafe to use std::array with union, C-style buffers is more safe.
        // Union need to be used because of Unix socket usage.
        /*
        Deviation from Rule A9-6-1:
        - ata types used for interfacing with hardware or conforming to communication protocols shall be trivial,
          standard-layout and only contain members of types with defined size
        Justification:
        - char array has predefined size kSocketCmsgSpace, thus false positive
        */
        // coverity[autosar_cpp14_a9_6_1_violation : FALSE]
        char buf[kSocketCmsgSpace];  // NOLINT(modernize-avoid-c-arrays) see comment above
        struct cmsghdr align;
        /*
        Deviation from Rule A9-5-1:
        - Unions shall not be used.
        Justification:
        - size of SocketCmsgSpace not equal to struct cmsghdr, padding is required
        */
        // coverity[autosar_cpp14_a9_5_1_violation]
    } u{};

    size_t num_fds = 0U;
    if (file_handle.has_value())
    {
        num_fds = 1U;

#if USE_SECURE_FILE_HANDLE_IPC
        std::cerr << "Passing file descriptors are not supported. Prepare and send shared memory file handle instead"
                  << std::endl;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) Union need to be used because of Unix socket usage.
        msg.msg_control = static_cast<char*>(u.buf);
        msg.msg_controllen = 0UL;
#else   //  USE_SECURE_FILE_HANDLE_IPC
        msg.msg_control = u.buf;
        msg.msg_controllen = sizeof(u.buf);

        struct cmsghdr* cmsg;
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg_len_suppress_warnings(cmsg, num_fds);
        std::int32_t* fdptr = cmsg_data_suppress_warning(cmsg);
        if (num_fds > 0)
        {
            fdptr[0] = file_handle.value();
        }
#endif  //  USE_SECURE_FILE_HANDLE_IPC
    }

    bool is_ping = (num_fds == 0) && (message.size() == 0);
    auto ret = score::os::Socket::instance().sendmsg(
        connection_file_descriptor,
        &msg,
        (is_ping ? score::os::Socket::MessageFlag::kWaitForOne : score::os::Socket::MessageFlag::kNone));

    if ((ret.has_value() == false) || (ret.value() == -1))
    {
        if (ret.error() == score::os::Error::Code::kResourceTemporarilyUnavailable)
        {
            static std::atomic_bool eagain_reported{false};
            if (true == eagain_reported.load())
            {
                return;
            }
            eagain_reported = true;
            std::perror("sendmsg");
        }
        else
        {
            std::cerr << "sendmsg: Error reported with errno: " << ret.error().ToString() << std::endl;
        }
    }
}

score::cpp::optional<std::string> recv_socket_message(std::int32_t socket_fd,
                                               AncillaryDataFileHandleReceptionCallback ancillary_data_process)
{
    score::cpp::optional<SharedMemoryFileHandle> discard_file_handle = score::cpp::nullopt;
    score::cpp::optional<std::int32_t> discard_pid = score::cpp::nullopt;
    return recv_socket_message(socket_fd, discard_file_handle, discard_pid, std::move(ancillary_data_process));
}

score::cpp::optional<std::string> recv_socket_message(std::int32_t socket_fd,
                                               score::cpp::optional<SharedMemoryFileHandle>& file_handle,
                                               score::cpp::optional<std::int32_t>& peer_pid,
                                               AncillaryDataFileHandleReceptionCallback ancillary_data_process)
{
    score::cpp::optional<std::string> result = score::cpp::nullopt;

    // TODO: currently checking viability. Redo in a safe way
    struct msghdr msg{};
    struct cmsghdr* cmsg{nullptr};
    std::int32_t* fdptr{nullptr};
    SocketMessangerHeader messanger_header{};

    iovec io{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) iovec is unchangeble, It's (POSIX standard)
    io.iov_base = &messanger_header;
    io.iov_len = sizeof(messanger_header);

    union
    {
        // It is unsafe to use std::array with union, C-style buffers is more safe.
        // Union need to be used because of Unix socket usage.
        /*
        Deviation from Rule A9-6-1:
        - ata types used for interfacing with hardware or conforming to communication protocols shall be trivial,
          standard-layout and only contain members of types with defined size
        Justification:
        - char array has predefined size kSocketCmsgSpace, thus false positive
        */
        // coverity[autosar_cpp14_a9_6_1_violation : FALSE]
        char buf[kSocketCmsgSpace];  // NOLINT(modernize-avoid-c-arrays) see comment above
        struct cmsghdr align;
        /*
        Deviation from Rule A9-5-1:
        - Unions shall not be used.
        Justification:
        - size of SocketCmsgSpace not equal to struct cmsghdr, padding is required
        */
        // coverity[autosar_cpp14_a9_5_1_violation]
    } u{};

    msg.msg_iov = &io;
    msg.msg_iovlen = 1U;
    // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) Union need to be used because of Unix socket usage
    msg.msg_control = static_cast<char*>(u.buf);
    msg.msg_controllen = sizeof(u.buf);
    // NOLINTEND(cppcoreguidelines-pro-type-union-access)
    /*
    Deviation from Rule M5-2-8:
    - An object with integer type or pointer to void type shall not be converted to an object with pointer type.
    Deviation from Rule M5-2-9:
    - A cast shall not convert a pointer type to an integral type
    Justification:
    - socket macro usage
    */
    // coverity[autosar_cpp14_m5_2_8_violation]
    // coverity[autosar_cpp14_m5_2_9_violation]
    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    size_t len_ = sizeof(std::int32_t) * static_cast<size_t>(1U);
    cmsg_len_suppress_warnings(cmsg, len_);
    fdptr = cmsg_data_suppress_warning(cmsg);
    static constexpr std::int32_t kNotAssigned = -1;
    score::cpp::span<std::int32_t>(fdptr, len_).front() = kNotAssigned;

    auto ret = score::os::Socket::instance().recvmsg(socket_fd, &msg, score::os::Socket::MessageFlag::kWaitAll);

    if ((ret.has_value() == false) || (ret.value() < 0))
    {
        if (ret.error() == score::os::Error::Code::kResourceTemporarilyUnavailable ||
            ret.error() == score::os::Error::Code::kOperationWasInterruptedBySignal)
        {  // timeout
            return std::string{};
        }
    }

    //  Return peer_pid value to the caller
    peer_pid = messanger_header.pid;

    std::array<char, 1024UL> iobuf{};
    if (ret.value() != static_cast<ssize_t>(sizeof(messanger_header)) ||
        static_cast<size_t>(messanger_header.len) > sizeof(iobuf))
    {
        std::cerr << "Unix Domain Socket communication is corrupted!, ret = " << ret.value() << std::endl;
        return score::cpp::nullopt;
    }
    // can not be tested since no control on fdptr.
    // LCOV_EXCL_START
    if (kNotAssigned != score::cpp::span<std::int32_t>(fdptr, len_).front())
    {
#if USE_SECURE_FILE_HANDLE_IPC
        std::cerr << "Warning: received handle over Socket Ancillary Message on QNX." << std::endl;
#endif  //  USE_SECURE_FILE_HANDLE_IPC
        file_handle = score::cpp::span<std::int32_t>(fdptr, len_).front();
    }
    // LCOV_EXCL_STOP

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access) iovec is unchangeble, It's (POSIX standard)
    io.iov_base = iobuf.data();
    io.iov_len = static_cast<uint32_t>(messanger_header.len);

    // coverity[autosar_cpp14_m19_3_1_violation] : see above
    errno = 0;
    ret = score::os::Socket::instance().recvmsg(socket_fd, &msg, score::os::Socket::MessageFlag::kWaitAll);
    if ((ret.has_value() == false) || (ret.value() < 0))
    {
        result = std::string{};
    }
    else
    {
        if (ret.value() > 0)
        {
            switch (messanger_header.type)
            {
                case MessageType::kDefault:
                    result = std::string{iobuf.data(), static_cast<size_t>(ret.value() - 1)};
                    break;
                case MessageType::kSharedMemoryFileHandle:

                    if (ancillary_data_process.empty() == false)
                    {
                        auto decoded = ancillary_data_process(
                            // coverity[autosar_cpp14_m5_2_8_violation] see above
                            score::cpp::span<std::uint8_t>{static_cast<std::uint8_t*>(static_cast<void*>(iobuf.data())),
                                                    static_cast<score::cpp::span<std::uint8_t>::size_type>(ret.value())});

                        if (decoded.has_value())
                        {
                            result = std::string{};  //  Pass the status of the operation as generally sucessful
                            if (file_handle.has_value())
                            {
                                std::cerr << "Overwriting file descriptor handle may lead to leaks" << std::endl;
                            }
                            file_handle = decoded;
                        }
                    }
                    break;
                default:
                    std::cerr << "UnixDomain: recvmsg Error" << std::endl;
                    break;
            }
        }
        else  //  ret == 0
        {
            result = score::cpp::nullopt;
        }
    }
    return result;
}

void SetupSignals(const std::unique_ptr<score::os::Signal>& signal)
{
    sigset_t sig_set{};
    auto res = signal->SigEmptySet(sig_set);
    if (!res.has_value())
    {
        std::perror(res.error().ToString().c_str());
    }
    res = signal->SigAddSet(sig_set, SIGTERM);
    if (!res.has_value())
    {
        std::perror(res.error().ToString().c_str());
    }
    // score::os wrappers are better suited for dependency injection,
    // So suppressed here as it is safely used, and it is among safety headers.
    // NOLINTNEXTLINE(score-banned-function) see comment above
    res = signal->PthreadSigMask(SIG_BLOCK, sig_set);
    if (!res.has_value())
    {
        std::perror(res.error().ToString().c_str());
    }

    struct sigaction sig_handler{};
    struct sigaction old_sigaction{};
    /*
    Deviation from Rule M5-2-9:
    - A cast shall not convert a pointer type to an integral type
    Justification: POSIX-compliant signal setup is required to safely ignore SIGPIPE and
        install controlled handlers for termination signals (SIGTERM, SIGINT).
    */
    // coverity[autosar_cpp14_m5_2_9_violation]
    sig_handler.sa_handler = SIG_IGN;
    // Need to fully initialize otherwise memchecker complains
    sig_handler.sa_mask = sig_set;
    sig_handler.sa_flags = 0;
    // score::os wrappers are better suited for dependency injection,
    // So suppressed here as it is safely used, and it is among safety headers.
    // NOLINTNEXTLINE(score-banned-function) see comment above
    res = signal->SigAction(SIGPIPE, sig_handler, old_sigaction);
    if (!res.has_value())
    {
        std::perror(res.error().ToString().c_str());
    }
}

}  // namespace internal
}  // namespace platform
}  // namespace score
