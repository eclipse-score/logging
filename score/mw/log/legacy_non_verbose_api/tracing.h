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

#ifndef SCORE_MW_LOG_LEGACY_NON_VERBOSE_API_TRACING_H
#define SCORE_MW_LOG_LEGACY_NON_VERBOSE_API_TRACING_H

/// \file This file contains the legacy API for non-verbose logging.
/// We only keep this file for legacy compatibility reasons.
/// Going forward a proper C++ API for mw::log shall be defined to replace this code.

#include "static_reflection_with_serialization/serialization/for_logging.h"

#include "score/os/utils/high_resolution_steady_clock.h"
#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/configuration/nvconfig.h"
#include "score/mw/log/detail/data_router/shared_memory/writer_factory.h"
#include "score/mw/log/detail/logging_identifier.h"
#include "score/mw/log/runtime.h"

namespace score
{
namespace platform
{
/*
Deviation from Rule A7-3-1
- All overloads of a function shall be visible from where it is called.
Justification:
- type alias used
*/
// coverity[autosar_cpp14_a7_3_1_violation]
using TimestampT = score::os::HighResolutionSteadyClock::time_point;
using MsgsizeT = uint16_t;

enum class LogLevel : uint8_t
{
    kOff = 0x00,
    kFatal = 0x01,
    kError = 0x02,
    kWarn = 0x03,
    kInfo = 0x04,
    kDebug = 0x05,
    kVerbose = 0x06
};

class Logger
{
  public:
    using AppPrefix = std::array<char, score::mw::log::detail::LoggingIdentifier::kMaxLength * 3UL>;

    static Logger& Instance(const score::cpp::optional<const score::mw::log::detail::Configuration>& config = score::cpp::nullopt,
                            std::optional<score::mw::log::NvConfig> nv_config = std::nullopt,
                            score::cpp::optional<score::mw::log::detail::SharedMemoryWriter> = score::cpp::nullopt) noexcept;

    explicit Logger(const score::cpp::optional<const score::mw::log::detail::Configuration>& config,
                    std::optional<score::mw::log::NvConfig> nv_config,
                    score::cpp::optional<score::mw::log::detail::SharedMemoryWriter>&&) noexcept;

    template <typename T>
    score::cpp::optional<score::mw::log::detail::TypeIdentifier> RegisterType() noexcept
    {
        class TypeinfoWithPrefix
        {
          public:
            explicit TypeinfoWithPrefix(const AppPrefix& app_pref) : app_prefix_(app_pref) {}
            std::size_t size() const
            {
                // LCOV_EXCL_START
                // This blocks checks and addresses an overflow situation although its
                // not technicaly possible. The logger_type_info<T>().size() is the size of a string containing
                // fully qualified name of the given C++ type. It is not feasible to simulate a string
                // as large as std::numeric_limits<size_t>::max() due to memory limitations. Hence this
                // block is excluded from LCOV.
                if (::score::common::visitor::logger_type_info<T>().size() >
                    std::numeric_limits<size_t>::max() - app_prefix_.size())
                {
                    // If an overflow were to happen, then its safe to return
                    // maxpayload size and appPrefixsize.
                    return app_prefix_.size() + score::mw::log::detail::SharedMemoryWriter::GetMaxPayloadSize();
                }
                // LCOV_EXCL_STOP

                return app_prefix_.size() + ::score::common::visitor::logger_type_info<T>().size();
            }
            void Copy(score::cpp::span<score::mw::log::detail::Byte> data) const
            {
                std::ignore = std::copy(app_prefix_.cbegin(), app_prefix_.cend(), data.begin());
                // The following condition should be always true
                if (data.size() >= app_prefix_.size())  // LCOV_EXCL_BR_LINE : see above
                {
                    const auto sub_data = data.subspan(app_prefix_.size());
                    ::score::common::visitor::logger_type_info<T>().copy(sub_data.data(), sub_data.size());
                }
            }

          private:
            const AppPrefix& app_prefix_;
        };

        if (shared_memory_writer_.has_value())
        {
            auto id = shared_memory_writer_.value().TryRegisterType(TypeinfoWithPrefix(app_prefix_));
            if (true == id.has_value())
            {
                return static_cast<score::mw::log::detail::TypeIdentifier>(id.value());
            }
        }

        return {};
    }

    template <typename T>
    LogLevel GetTypeLevel() const
    {
        auto log_level = LogLevel::kInfo;
        const score::mw::log::config::NvMsgDescriptor* const msg_desc =
            nvconfig_.GetDltMsgDesc(::score::common::visitor::struct_visitable<T>::name());
        if (msg_desc != nullptr)
        {
            auto message_descriptor_log_level = msg_desc->GetLogLevel();
            // Check the range's value of the log level before casting it to the LogLevel enum.
            if (message_descriptor_log_level <=
                score::mw::log::LogLevel::kVerbose)  // LCOV_EXCL_BR_START  message_descriptor_log_level is enum class
                                                   // which is constant & can't exceed more than, kVerbose.
            {
                // LCOV_EXCL_BR_STOP
                log_level = static_cast<LogLevel>(msg_desc->GetLogLevel());
            }
        }
        return log_level;
    }

    template <typename T>
    LogLevel GetTypeThreshold() const noexcept
    {
        return GetLevelForContext(::score::common::visitor::struct_visitable<T>::name()).value_or(LogLevel::kVerbose);
    }

    score::mw::log::detail::SharedMemoryWriter& GetSharedMemoryWriter();

    const score::mw::log::detail::Configuration& GetConfig() const;

    /// \brief Only for testing to inject an instance to intercept and check the behavior.
    static void InjectTestInstance(Logger* const logger_ptr);

  private:
    static Logger** GetInjectedTestInstance();
    std::optional<LogLevel> GetLevelForContext(const std::string& name) const noexcept;

    score::mw::log::detail::Configuration config_;
    const score::mw::log::NvConfig nvconfig_;
    score::cpp::optional<score::mw::log::detail::SharedMemoryWriter> shared_memory_writer_;
    score::mw::log::detail::SharedData discard_operation_fallback_shm_data_;
    score::mw::log::detail::SharedMemoryWriter discard_operation_fallback_shm_writer_;
    AppPrefix app_prefix_;
};

template <typename T>
class LogEntry
{
  public:
    static LogEntry& Instance() noexcept
    {
        // It's a singleton by design hence cannot be made const
        // coverity[autosar_cpp14_a3_3_2_violation]
        static LogEntry entry{};
        return entry;
    }

    std::optional<score::mw::log::detail::TypeIdentifier> RegisterTypeGetId() noexcept
    {
        const auto registered_id = Logger::Instance().RegisterType<T>();

        // If registration failed, return empty optional
        if (!registered_id.has_value())
        {
            return std::nullopt;
        }

        return std::optional<score::mw::log::detail::TypeIdentifier>{UpdateSharedMemoryId(registered_id.value())};
    }

    score::mw::log::detail::TypeIdentifier UpdateSharedMemoryId(
        score::mw::log::detail::TypeIdentifier registered_id) noexcept
    {
        // Use compare_exchange to safely update from registration token to actual ID
        // Only one thread should successfully update, others will see the already-updated value
        auto expected = score::mw::log::detail::GetRegisterTypeToken();
        const bool successfully_updated = shared_memory_id_.compare_exchange_strong(expected, registered_id);

        // Return the actual ID: newly updated value if successful, or current value if another thread updated it
        if (successfully_updated)
        {
            return registered_id;
        }
        // LCOV_EXCL_START
        // Rationale for coverage exclusion:
        // This else branch is triggered when compare_exchange_strong fails, which occurs in a
        // multi-threaded race condition where two threads attempt to register the same type
        // simultaneously. For this to happen:
        // 1. Both threads must pass the guard check (shared_memory_id_ == registration_token)
        // 2. Thread A executes compare_exchange_strong and succeeds (updates shared_memory_id_)
        // 3. Thread B executes compare_exchange_strong and fails (shared_memory_id_ already updated)
        //
        // Why this cannot be reliably tested in unit tests:
        // - The race window exists only between the guard check and the atomic operation
        // - This window is typically a few CPU cycles (nanoseconds)
        // - Once any thread updates shared_memory_id_, the guard check prevents other threads
        //   from reaching compare_exchange_strong
        // - Unit tests cannot deterministically create this precise timing condition without
        //   introducing test flakiness
        // - Thread scheduling is non-deterministic and hardware-dependent
        //
        // The correctness is guaranteed by C++ atomics: when compare_exchange_strong fails,
        // 'expected' is automatically updated with the current shared_memory_id_ value, which
        // is then correctly returned.
        else
        {
            return expected;
        }
        // LCOV_EXCL_STOP
    }

    template <typename F>
    void TrySerializeIntoSharedMemory(F serialize) noexcept
    {
        if (score::mw::log::detail::GetRegisterTypeToken() == shared_memory_id_)
        {
            if (!RegisterTypeGetId().has_value())
            {
                dropped_logs_due_to_failed_registration_.fetch_add(1U, std::memory_order_relaxed);
                Logger::Instance().GetSharedMemoryWriter().IncrementTypeRegistrationFailures();
                return;
            }
        }
        serialize();
    }

    void TryWriteIntoSharedMemory(const T& t) noexcept
    {
        TrySerializeIntoSharedMemory([&t, this]() noexcept {
            using S = ::score::common::visitor::logging_serializer;
            Logger::Instance().GetSharedMemoryWriter().AllocAndWrite(
                [&t](const auto data_span) {
                    return S::serialize(t, data_span.data(), data_span.size());
                },
                shared_memory_id_,
                static_cast<uint64_t>(S::serialize_size(t)));
        });
    }

    /// \public
    /// \thread-safe
    void LogAtTime(TimestampT timestamp, const T& t)
    {
        TrySerializeIntoSharedMemory([&timestamp, &t, this]() {
            using S = ::score::common::visitor::logging_serializer;
            Logger::Instance().GetSharedMemoryWriter().AllocAndWrite(
                timestamp, shared_memory_id_, S::serialize_size(t), [&t](const auto data_span) {
                    return S::serialize(t, data_span.data(), data_span.size());
                });
        });
    }

    static LogLevel GetThresholdForInit() noexcept
    {
        return Logger::Instance().GetTypeThreshold<T>();
    }

    static bool GetDefaultEnabledForInit() noexcept
    {
        const auto threshold = Logger::Instance().GetTypeThreshold<T>();
        const auto level = Logger::Instance().GetTypeLevel<T>();
        return threshold >= level;
    }

    static score::mw::log::detail::TypeIdentifier GetInitialTypeId() noexcept
    {
        const auto registered_id = Logger::Instance().RegisterType<T>();
        return registered_id.value_or(score::mw::log::detail::GetRegisterTypeToken());
    }

    LogEntry() noexcept
        : recorder_{score::mw::log::detail::Runtime::GetRecorder()},
          shared_memory_id_{GetInitialTypeId()},
          default_enabled_{GetDefaultEnabledForInit()},
          level_enabled_{GetThresholdForInit()},
          dropped_logs_due_to_failed_registration_{0U}
    {
        static_assert(sizeof(typename ::score::common::visitor::logging_serialized_descriptor<T>::payload_type) <=
                          score::mw::log::detail::SharedMemoryWriter::GetMaxPayloadSize(),
                      "Serialized type too large");
        static_assert(std::atomic<score::mw::log::detail::TypeIdentifier>::is_always_lock_free,
                      "TypeIdentifier must be lock-free for thread-safe logging");
    }

    /// \public
    /// \thread-safe
    void LogSerialized(const char* data, const MsgsizeT size)
    {
        TrySerializeIntoSharedMemory([&data, &size, this]() {
            Logger::Instance().GetSharedMemoryWriter().AllocAndWrite(
                [&data, &size](const auto data_span) {
                    const auto data_pointer = data_span.data();
                    std::ignore = std::copy_n(data, size, data_pointer);
                    return size;
                },
                shared_memory_id_,
                size);
        });
    }

    /// \public
    /// \thread-safe
    bool Enabled() const
    {
        return default_enabled_;
    }

    /// \public
    /// \thread-safe
    bool EnabledAt(LogLevel level) const
    {
        return level_enabled_ >= level;
    }

    /// \public
    /// \thread-safe
    /// \brief Returns the number of log entries dropped due to failed type registration
    std::uint64_t GetDroppedLogsCount() const noexcept
    {
        return dropped_logs_due_to_failed_registration_.load(std::memory_order_relaxed);
    }

  private:
    score::mw::log::Recorder& recorder_;  // Used only to place a call to GetRecorder() on the initialization list.
    std::atomic<score::mw::log::detail::TypeIdentifier> shared_memory_id_;
    const bool default_enabled_;
    const LogLevel level_enabled_;
    std::atomic<std::uint64_t> dropped_logs_due_to_failed_registration_;
};

}  // namespace platform
}  // namespace score

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline auto& GetLogEntry()
{
    using DecayedType = std::decay_t<T>;
    return score::platform::LogEntry<DecayedType>::Instance();
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceLevel(score::platform::LogLevel level, const T& arg)
{
    auto& logger = GetLogEntry<T>();
    if (logger.EnabledAt(level))
    {
        logger.TryWriteIntoSharedMemory(arg);
    }
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void LogInternalLogger(const T& arg)
{
    auto& logger = GetLogEntry<T>();
    if (logger.Enabled())
    {
        logger.TryWriteIntoSharedMemory(arg);
    }
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TRACE(const T& arg)
{
    LogInternalLogger(arg);
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceVerbose(const T& arg)
{
    TraceLevel(score::platform::LogLevel::kVerbose, arg);
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceDebug(const T& arg)
{
    TraceLevel(score::platform::LogLevel::kDebug, arg);
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceInfo(const T& arg)
{
    TraceLevel(score::platform::LogLevel::kInfo, arg);
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceWarning(const T& arg)
{
    TraceLevel(score::platform::LogLevel::kWarn, arg);
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceError(const T& arg)
{
    TraceLevel(score::platform::LogLevel::kError, arg);
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceFatal(const T& arg)
{
    TraceLevel(score::platform::LogLevel::kFatal, arg);
}

/// \public
/// \thread-safe
template <typename T>
// Defined in global namespace, because it is global function, used in many files in different namespaces.
// coverity[autosar_cpp14_m7_3_1_violation]
inline void TraceWarn(const T& arg)
{
    TraceLevel(score::platform::LogLevel::kWarn, arg);
}

/// \public
/// \thread-safe
// Using preprocessor here because we define function macro to use it any where with name STRUCT_TRACEABLE
// coverity[autosar_cpp14_a16_0_1_violation]
#define STRUCT_TRACEABLE(...) STRUCT_VISITABLE(__VA_ARGS__)

#endif  // SCORE_MW_LOG_LEGACY_NON_VERBOSE_API_TRACING_H
