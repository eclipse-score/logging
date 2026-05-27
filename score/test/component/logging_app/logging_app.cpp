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

#include <unistd.h>
#include <climits>
#include <cstdint>
#include <string>

namespace mw_log = score::mw::log;

namespace {

void DoLogging() {
    const std::string SWID{"Logging Application"};
    mw_log::LogInfo() << SWID << "DoLogging";
    mw_log::LogInfo() << "val_bool" << true;

    // Unsigned integers
    const uint8_t val_uint8t = 123;
    const uint16_t val_uint16t = 1234;
    const uint32_t val_uint32t = 12345;
    const uint64_t val_uint64t = 123456;
    mw_log::LogDebug() << "val_uint8t" << val_uint8t << "val_uint16t" << val_uint16t
                    << "val_uint32t" << val_uint32t << "val_uint64t" << val_uint64t;

    mw_log::LogWarn() << "val_uint8tmax" << UINT8_MAX << "val_uint16tmax" << UINT16_MAX
                   << "val_uint32tmax" << UINT32_MAX << "val_uint64tmax" << UINT64_MAX;

    // Signed integers
    const int8_t val_int8t = -34;
    const int16_t val_int16t = -14576;
    const int32_t val_int32t = -2147483640;
    const int64_t val_int64t = -9223372036854775700LL;
    mw_log::LogFatal() << "val_int8t" << val_int8t << "val_int16t" << val_int16t
                    << "val_int32t" << val_int32t << "val_int64t" << val_int64t;

    mw_log::LogWarn() << "val_int8tmax" << INT8_MAX << "val_int16tmax" << INT16_MAX
                   << "val_int32tmax" << INT32_MAX << "val_int64tmax" << INT64_MAX;

    mw_log::LogDebug() << "val_uint8t" << val_uint8t << "val_uint16t" << val_uint16t
                    << "val_uint32t" << val_uint32t << "val_uint64t" << val_uint64t;

    mw_log::LogWarn() << "val_int8tmin" << INT8_MIN << "val_int16tmin" << INT16_MIN
                   << "val_int32tmin" << INT32_MIN << "val_int64tmin" << INT64_MIN;

    const int32_t val_int8tminplusint8t = static_cast<int32_t>(INT8_MIN) - val_int8t;
    const int32_t val_int16tminplusint16t = static_cast<int32_t>(INT16_MIN) - val_int16t;
    const int32_t val_int32tminplusint32t = INT32_MIN - val_int32t;
    const int64_t val_int64tminplusint64t = INT64_MIN - val_int64t;
    mw_log::LogError() << "val_int8tminplusint8t" << val_int8tminplusint8t
                    << "val_int16tminplusint16t" << val_int16tminplusint16t
                    << "val_int32tminplusint32t" << val_int32tminplusint32t
                    << "val_int64tminplusint64t" << val_int64tminplusint64t;

    // String and double
    mw_log::LogFatal() << "val_string" << "Logging";
    mw_log::LogFatal() << "val_double" << 93454.6;

    // Hex values
    mw_log::LogInfo() << "log_hex_8" << mw_log::LogHex8{10}
                   << "log_hex_16" << mw_log::LogHex16{9876}
                   << "log_hex_32" << mw_log::LogHex32{543210987}
                   << "log_hex_64" << mw_log::LogHex64{654321098765432109ULL};

    // Bin values
    mw_log::LogWarn() << "log_bin_8" << mw_log::LogBin8{8}
                   << "log_bin_16" << mw_log::LogBin16{9012}
                   << "log_bin_32" << mw_log::LogBin32{3456789012UL}
                   << "log_bin_64" << mw_log::LogBin64{3456789012345678901ULL};

    // Raw buffer
    mw_log::LogFatal() << "log_raw_buffer" << mw_log::LogRawBuffer{"raw", 3};

    // Slog2 message
    mw_log::LogFatal() << "log_slog2_message" << mw_log::LogSlog2Message{11, "slog2_message"};
}

}  // namespace

int main() {
    DoLogging();
    sleep(1);
    return 0;
}
