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

#ifndef DLT_HEADERS_H_
#define DLT_HEADERS_H_

#include "dlt/dlt_common.h"
#include "score/mw/log/detail/common/log_entry_deserialize.h"
#include "score/mw/log/detail/log_entry.h"
#include "score/datarouter/include/dlt/dltid_converter.h"

namespace score
{
namespace platform
{
namespace internal
{

static constexpr uint32_t DLT_MESSAGE_SIZE = 65536UL;

// NOLINTBEGIN(score-banned-preprocessor-directives) see below
/**
 * Suppress compiler warnings related to Wpacked and Wattributes
 * because some structures declared with PACKED attribute
 * if this attribute has no effect it causes warning
 */
#if defined(__INTEL_COMPILER)  // ICC warnings not complete
// required due to ICC warnings
// coverity[autosar_cpp14_a16_7_1_violation]
#pragma warning(push)
#elif defined(__GNUC__)
// required due to ICC warnings
// coverity[autosar_cpp14_a16_7_1_violation]
#pragma GCC diagnostic push
// required due to ICC warnings
// coverity[autosar_cpp14_a16_7_1_violation]
#pragma GCC diagnostic ignored "-Wpacked"
// required due to ICC warnings
// coverity[autosar_cpp14_a16_7_1_violation]
#pragma GCC diagnostic ignored "-Wattributes"
#endif

typedef struct
{
    DltStandardHeader std;
    DltStandardHeaderExtra stde;
    uint32_t msgid;
} PACKED DltNvHeaderWithMsgid;

typedef struct
{
    DltStandardHeader std;
    DltStandardHeaderExtra stde;
    DltExtendedHeader ext;
} PACKED DltVerboseHeader;

#if defined(__INTEL_COMPILER)
// required due to ICC warnings
// coverity[autosar_cpp14_a16_7_1_violation]
#pragma warning(pop)
#elif defined(__GNUC__)
// required due to ICC warnings
// coverity[autosar_cpp14_a16_7_1_violation]
#pragma GCC diagnostic pop
#endif
// NOLINTEND(score-banned-preprocessor-directives)

inline void ConstructDltStorageHeader(DltStorageHeader& storagehdr, uint32_t secs, int32_t microsecs)
{
    storagehdr.pattern[0] = 'D';
    storagehdr.pattern[1] = 'L';
    storagehdr.pattern[2] = 'T';
    storagehdr.pattern[3] = 0x01;
    storagehdr.seconds = secs;
    storagehdr.microseconds = microsecs;
    storagehdr.ecu[0] = 'E';
    storagehdr.ecu[1] = 'C';
    storagehdr.ecu[2] = 'U';
    storagehdr.ecu[3] = '\0';
}

inline void ConstructDltStandardHeader(DltStandardHeader& std, size_t msg_size, uint8_t mcnt, bool useextheader = false)
{
    std.htyp = static_cast<uint8_t>(DLT_HTYP_WEID | DLT_HTYP_WTMS | DLT_HTYP_VERS);
    if (useextheader)
    {
        std.htyp |= static_cast<uint8_t>(DLT_HTYP_UEH);
    }
    std.mcnt = mcnt;
    std.len = DLT_HTOBE_16(msg_size);
}
inline void ConstructDltStandardHeaderExtra(DltStandardHeaderExtra& stde, dltid_t ecu, uint32_t tmsp)
{
    std::copy_n(ecu.data(), std::min(ecu.size(), sizeof(stde.ecu)), static_cast<char*>(stde.ecu));
    stde.tmsp = DLT_HTOBE_32(tmsp);
}

inline void ConstructDltExtendedHeader(DltExtendedHeader& ext,
                                       const mw::log::LogLevel loglevel,
                                       uint8_t nor,
                                       dltid_t appId,
                                       dltid_t ctxId)
{
    ext.msin = static_cast<uint8_t>((DLT_TYPE_LOG << DLT_MSIN_MSTP_SHIFT) |
                                    (static_cast<std::uint8_t>(loglevel) << DLT_MSIN_MTIN_SHIFT) | (DLT_MSIN_VERB));
    ext.noar = nor;
    std::copy_n(appId.data(), sizeof(ext.apid), static_cast<char*>(ext.apid));
    std::copy_n(ctxId.data(), sizeof(ext.ctid), static_cast<char*>(ext.ctid));
}

inline void ConstructStorageVerbosePacket(uint8_t* dlt_message,
                                          const score::mw::log::detail::LogEntry& entry,
                                          dltid_t ecu,
                                          uint8_t mcnt,
                                          uint32_t tmsp,
                                          uint32_t secs,
                                          int32_t microsecs)
{
    // truncate the message to max size if the msg size is exceeding the available buffer size
    size_t size =
        std::min(entry.payload.size(), DLT_MESSAGE_SIZE - (sizeof(DltStorageHeader) + sizeof(DltVerboseHeader)));
    auto& storagehdr = *static_cast<DltStorageHeader*>(static_cast<void*>(dlt_message));
    ConstructDltStorageHeader(storagehdr, secs, microsecs);
    auto& hdr = *static_cast<DltVerboseHeader*>(static_cast<void*>(std::next(
        dlt_message,
        static_cast<typename std::iterator_traits<decltype(dlt_message)>::difference_type>(sizeof(DltStorageHeader)))));
    ConstructDltStandardHeader(hdr.std, (sizeof(DltVerboseHeader) + size), mcnt, true);
    ConstructDltStandardHeaderExtra(hdr.stde, ecu, tmsp);
    ConstructDltExtendedHeader(hdr.ext,
                               entry.log_level,
                               static_cast<uint8_t>(entry.num_of_args),
                               platform::convertToDltId(entry.app_id),
                               platform::convertToDltId(entry.ctx_id));
    // avoid signed/unsigned comparison warning in std::next
    const auto offset = static_cast<typename std::iterator_traits<decltype(dlt_message)>::difference_type>(
        sizeof(DltStorageHeader) + sizeof(DltVerboseHeader));
    std::copy_n(entry.payload.data(), size, std::next(dlt_message, offset));
}

inline uint32_t ConstructVerboseHeader(
    DltVerboseHeader& header,
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry,
    dltid_t ecu,
    uint8_t mcnt,
    uint32_t tmsp)
{
    size_t payload_size = std::min(static_cast<std::uint32_t>(entry.GetPayload().size()),
                                   static_cast<std::uint32_t>(DLT_MESSAGE_SIZE - sizeof(DltVerboseHeader)));
    ConstructDltStandardHeader(header.std, (sizeof(DltVerboseHeader) + payload_size), mcnt, true);
    ConstructDltStandardHeaderExtra(header.stde, ecu, tmsp);
    ConstructDltExtendedHeader(header.ext,
                               entry.log_level,
                               static_cast<uint8_t>(entry.num_of_args),
                               platform::convertToDltId(entry.app_id),
                               platform::convertToDltId(entry.ctx_id));
    return sizeof(DltVerboseHeader);
}

inline void ConstructVerbosePacket(
    char* dlt_message,
    const score::mw::log::detail::log_entry_deserialization::LogEntryDeserializationReflection& entry,
    dltid_t ecu,
    uint8_t mcnt,
    uint32_t tmsp)
{
    // truncate the message to max size if the msg size is exceeding the available buffer size
    const auto data_size = static_cast<size_t>(entry.GetPayload().size());
    std::size_t size = std::min(data_size, DLT_MESSAGE_SIZE - sizeof(DltVerboseHeader));
    // autosar_cpp14_m5_2_8_violation : No harm from casting void pointer
    // coverity[autosar_cpp14_m5_2_8_violation]
    auto& hdr = *static_cast<DltVerboseHeader*>(static_cast<void*>(dlt_message));
    // avoid signed/unsigned comparison warning in std::next
    const auto constructed_header_size =
        static_cast<typename std::iterator_traits<decltype(dlt_message)>::difference_type>(
            ConstructVerboseHeader(hdr, entry, ecu, mcnt, tmsp));
    std::copy_n(static_cast<const char*>(static_cast<const void*>(entry.GetPayload().data())),
                size,
                std::next(dlt_message, constructed_header_size));
}

inline uint32_t ConstructNonVerboseHeader(DltNvHeaderWithMsgid& hdr,
                                          size_t size,
                                          uint32_t msgid,
                                          dltid_t ecu,
                                          uint8_t mcnt,
                                          uint32_t tmsp)
{
    ConstructDltStandardHeader(hdr.std, (sizeof(DltNvHeaderWithMsgid) + size), mcnt);
    ConstructDltStandardHeaderExtra(hdr.stde, ecu, tmsp);
    hdr.msgid = msgid;
    return sizeof(DltNvHeaderWithMsgid);
}

inline void ConstructNonVerbosePacket(char* dlt_message,
                                      const void* data,
                                      size_t size,
                                      uint32_t msgid,
                                      dltid_t ecu,
                                      uint8_t mcnt,
                                      uint32_t tmsp)
{
    // autosar_cpp14_m5_2_8_violation : No harm from casting void pointer
    // coverity[autosar_cpp14_m5_2_8_violation]
    auto& hdr = *static_cast<DltNvHeaderWithMsgid*>(static_cast<void*>(dlt_message));
    // avoid signed/unsigned comparison warning in std::next
    const auto constructed_header_size =
        static_cast<typename std::iterator_traits<decltype(dlt_message)>::difference_type>(
            ConstructNonVerboseHeader(hdr, size, msgid, ecu, mcnt, tmsp));
    std::copy_n(static_cast<const char*>(data), size, std::next(dlt_message, constructed_header_size));
}

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // DLT_HEADERS_H_
