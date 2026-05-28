// *******************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

#include "score/mw/log/logging.h"

#include <unistd.h>

#include <cstdint>
#include <limits>
#include <string>

namespace log = score::mw::log;

int main()
{
    // Boolean
    log::LogInfo() << true;
    log::LogInfo() << false;

    // Unsigned integers
    log::LogInfo() << static_cast<std::uint8_t>(255U);
    log::LogInfo() << static_cast<std::uint16_t>(65535U);
    log::LogInfo() << static_cast<std::uint32_t>(4294967295UL);
    log::LogInfo() << std::numeric_limits<std::uint64_t>::max();

    // Signed integers
    log::LogInfo() << static_cast<std::int8_t>(-128);
    log::LogInfo() << static_cast<std::int16_t>(-32768);
    log::LogInfo() << static_cast<std::int32_t>(-2147483648L);
    log::LogInfo() << std::numeric_limits<std::int64_t>::min();

    // Min values
    log::LogInfo() << std::numeric_limits<std::uint8_t>::min();
    log::LogInfo() << std::numeric_limits<std::uint16_t>::min();
    log::LogInfo() << std::numeric_limits<std::uint32_t>::min();
    log::LogInfo() << std::numeric_limits<std::uint64_t>::min();

    log::LogInfo() << std::numeric_limits<std::int8_t>::max();
    log::LogInfo() << std::numeric_limits<std::int16_t>::max();
    log::LogInfo() << std::numeric_limits<std::int32_t>::max();
    log::LogInfo() << std::numeric_limits<std::int64_t>::max();

    // String
    log::LogInfo() << std::string("test string");

    // Double
    log::LogInfo() << 3.14;

    // Hex
    log::LogInfo() << log::LogHex8(0xABU);
    log::LogInfo() << log::LogHex16(0xABCDU);
    log::LogInfo() << log::LogHex32(0xABCDEF01UL);
    log::LogInfo() << log::LogHex64(0xABCDEF0123456789ULL);

    // Binary
    log::LogInfo() << log::LogBin8(0b10101010U);
    log::LogInfo() << log::LogBin16(0b1010101010101010U);
    log::LogInfo() << log::LogBin32(0b10101010101010101010101010101010UL);
    log::LogInfo() << log::LogBin64(0b1010101010101010101010101010101010101010101010101010101010101010ULL);

    // Raw buffer
    log::LogInfo() << log::LogRawBuffer{"raw", 3};

    // Slog2 message
    log::LogInfo() << log::LogSlog2Message{11, "slog2_message"};

    sleep(1);

    return 0;
}
