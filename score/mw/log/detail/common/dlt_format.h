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
#ifndef SCORE_MW_LOG_DETAIL_COMMON_DLT_FORMAT_H
#define SCORE_MW_LOG_DETAIL_COMMON_DLT_FORMAT_H

#include "score/mw/log/detail/add_argument_result.h"
#include "score/mw/log/detail/integer_representation.h"
#include "score/mw/log/detail/verbose_payload.h"
#include "score/mw/log/log_types.h"

#include <string_view>

#include <cstdint>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

using score::mw::log::LogBin16;
using score::mw::log::LogBin32;
using score::mw::log::LogBin64;
using score::mw::log::LogBin8;
using score::mw::log::LogHex16;
using score::mw::log::LogHex32;
using score::mw::log::LogHex64;
using score::mw::log::LogHex8;
using score::mw::log::LogRawBuffer;
using score::mw::log::detail::AddArgumentResult;
using score::mw::log::detail::Byte;
using score::mw::log::detail::ByteVector;
using score::mw::log::detail::IntegerRepresentation;
using score::mw::log::detail::VerbosePayload;
class DLTFormat
{
  public:
    static AddArgumentResult Log(VerbosePayload&, const bool) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::uint8_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::uint16_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::uint32_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::uint64_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::int8_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::int16_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::int32_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const std::int64_t,
                                 const IntegerRepresentation = IntegerRepresentation::kDecimal) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogHex8,
                                 const IntegerRepresentation = IntegerRepresentation::kHex) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogHex16,
                                 const IntegerRepresentation = IntegerRepresentation::kHex) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogHex32,
                                 const IntegerRepresentation = IntegerRepresentation::kHex) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogHex64,
                                 const IntegerRepresentation = IntegerRepresentation::kHex) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogBin8,
                                 const IntegerRepresentation = IntegerRepresentation::kBinary) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogBin16,
                                 const IntegerRepresentation = IntegerRepresentation::kBinary) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogBin32,
                                 const IntegerRepresentation = IntegerRepresentation::kBinary) noexcept;
    static AddArgumentResult Log(VerbosePayload&,
                                 const LogBin64,
                                 const IntegerRepresentation = IntegerRepresentation::kBinary) noexcept;
    static AddArgumentResult Log(VerbosePayload&, const float) noexcept;
    static AddArgumentResult Log(VerbosePayload&, const double) noexcept;
    static AddArgumentResult Log(VerbosePayload&, const std::string_view) noexcept;
    static AddArgumentResult Log(VerbosePayload&, const LogRawBuffer) noexcept;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_COMMON_DLT_FORMAT_H
