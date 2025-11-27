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

#include "score/mw/log/logging.h"
#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/logger.h"
#include "score/mw/log/log_level.h"

namespace score::mw::log
{
    extern "C"
    {

        Logger *mw_log_create_logger(const char *context)
        {
            return &CreateLogger(context);
        }

        bool mw_log_is_log_level_enabled_internal(const Logger *logger, uint8_t level)
        {
            return logger->IsLogEnabled(GetLogLevelFromU8(level));
        }

        void mw_log_fatal_logger(const Logger *logger, const char *message, uint32_t size)
        {
            logger->LogFatal() << LogString{message, size};
        }

        void mw_log_error_logger(const Logger *logger, const char *message, uint32_t size)
        {
            logger->LogError() << LogString{message, size};
        }

        void mw_log_warn_logger(const Logger *logger, const char *message, uint32_t size)
        {
            logger->LogWarn() << LogString{message, size};
        }

        void mw_log_info_logger(const Logger *logger, const char *message, uint32_t size)
        {
            logger->LogInfo() << LogString{message, size};
        }

        void mw_log_debug_logger(const Logger *logger, const char *message, uint32_t size)
        {
            logger->LogDebug() << LogString{message, size};
        }

        void mw_log_verbose_logger(const Logger *logger, const char *message, uint32_t size)
        {
            logger->LogVerbose() << LogString{message, size};
        }

        uint8_t mw_log_logger_level_internal(const Logger *logger)
        {
            // TODO: This is adapter code, as there seems to be no way to get log level for Logger
            if (logger->IsLogEnabled(LogLevel::kInfo))
            {
                // Between Verbose, Debug, Info
                if (logger->IsLogEnabled(LogLevel::kDebug))
                {
                    if (logger->IsLogEnabled(LogLevel::kVerbose))
                    {
                        return static_cast<uint8_t>(LogLevel::kVerbose);
                    }

                    return static_cast<uint8_t>(LogLevel::kDebug);
                }

                return static_cast<uint8_t>(LogLevel::kInfo);
            }
            else
            {
                // Lower half: Warn, Error, Fatal
                if (logger->IsLogEnabled(LogLevel::kError))
                {
                    if (logger->IsLogEnabled(LogLevel::kWarn))
                    {
                        return static_cast<uint8_t>(LogLevel::kWarn);
                    }

                    return static_cast<uint8_t>(LogLevel::kError);
                }

                if (logger->IsLogEnabled(LogLevel::kFatal))
                {
                    return static_cast<uint8_t>(LogLevel::kFatal);
                }
            }

            // fallback
            return static_cast<uint8_t>(LogLevel::kOff);
        }
    }
} // namespace score::mw::log
