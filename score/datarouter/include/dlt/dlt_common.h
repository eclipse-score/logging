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
/*
 * @licence app begin@
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2011-2015, BMW AG
 *
 * This file is part of GENIVI Project DLT - Diagnostic Log and Trace.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*******************************************************************************
**                      Revision Control History                              **
*******************************************************************************/

/*
 * $LastChangedRevision: 1670 $
 * $LastChangedDate: 2011-04-08 15:12:06 +0200 (Fr, 08. Apr 2011) $
 * $LastChangedBy$
 Initials    Date         Comment
 aw          13.01.2010   initial
 */
#ifndef DLT_COMMON_H
#define DLT_COMMON_H

/**
  \defgroup commonapi DLT Common API
  \addtogroup commonapi
  \{
*/

#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <climits>

#include "dlt_protocol.h"
#include "dlt_types.h"

#if !defined(PACKED)
#define PACKED __attribute__((aligned(1), packed))
#endif

/*
 * Macros to swap the byte order.
 */
#define DLT_SWAP_16(value) static_cast<uint16_t>((((value) >> 8) & 0xff) | (((value) << 8) & 0xff00))

#define DLT_SWAP_32(value)                                                                                     \
    static_cast<uint32_t>((((value) >> 24) & 0xff) | (((value) << 8) & 0xff0000) | (((value) >> 8) & 0xff00) | \
                          (((value) << 24) & 0xff000000))

#define DLT_SWAP_64(value)                                                                      \
    static_cast<uint64_t>((static_cast<uint64_t>(DLT_SWAP_32((value) & 0xffffffffull)) << 32) | \
                          (DLT_SWAP_32((value) >> 32)))

/* #warning "Litte Endian Architecture!" */
#define DLT_HTOBE_16(x) DLT_SWAP_16((x))
#define DLT_HTOLE_16(x) ((x))
#define DLT_BETOH_16(x) DLT_SWAP_16((x))
#define DLT_LETOH_16(x) ((x))

#define DLT_HTOBE_32(x) DLT_SWAP_32((x))
#define DLT_HTOLE_32(x) ((x))
#define DLT_BETOH_32(x) DLT_SWAP_32((x))
#define DLT_LETOH_32(x) ((x))

#define DLT_HTOBE_64(x) DLT_SWAP_64((x))
#define DLT_HTOLE_64(x) ((x))
#define DLT_BETOH_64(x) DLT_SWAP_64((x))
#define DLT_LETOH_64(x) ((x))

/**
 * The size of a DLT ID
 */
#define DLT_ID_SIZE 4

// NOLINTBEGIN(score-banned-preprocessor-directives) see below
/**
 * Suppress compiler warnings related to Wpacked and Wattributes
 * because some structures delared with PACKED attribute
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
// NOLINTEND(score-banned-preprocessor-directives)

// NOLINTBEGIN(modernize-avoid-c-arrays) see below
/**
 * dlt_common.h header part of GENIVI Project DLT, provide C-style interface and should not be changed by user
 * So plain array could not be replaced by modern alternatives
 */
/**
 * The structure of the DLT file storage header. This header is used before each stored DLT message.
 */
typedef struct
{
    // coverity[autosar_cpp14_a9_6_1_violation] False positive, size is well defined DLT_ID_SIZE -> 4
    char pattern[DLT_ID_SIZE]; /**< This pattern should be DLT0x01 */
    uint32_t seconds;          /**< seconds since 1.1.1970 */
    int32_t microseconds;      /**< Microseconds */
    // coverity[autosar_cpp14_a9_6_1_violation] False positive, size is well defined DLT_ID_SIZE -> 4
    char ecu[DLT_ID_SIZE]; /**< The ECU id is added, if it is not already in the DLT message itself */
} PACKED DltStorageHeader;

/**
 * The structure of the DLT standard header. This header is used in each DLT message.
 */
typedef struct
{
    uint8_t htyp; /**< This parameter contains several informations, see definitions below */
    uint8_t mcnt; /**< The message counter is increased with each sent DLT message */
    uint16_t len; /**< Length of the complete message, without storage header */
} PACKED DltStandardHeader;

/**
 * The structure of the DLT extra header parameters. Each parameter is sent only if enabled in htyp.
 */
typedef struct
{
    // coverity[autosar_cpp14_a9_6_1_violation] False positive, size is well defined DLT_ID_SIZE -> 4
    char ecu[DLT_ID_SIZE]; /**< ECU id */
    uint32_t tmsp;         /**< Timestamp since system start in 0.1 milliseconds */
} PACKED DltStandardHeaderExtra;

/**
 * The structure of the DLT extended header. This header is only sent if enabled in htyp parameter.
 */
typedef struct
{
    uint8_t msin; /**< messsage info */
    uint8_t noar; /**< number of arguments */
    // coverity[autosar_cpp14_a9_6_1_violation] False positive, size is well defined DLT_ID_SIZE -> 4
    char apid[DLT_ID_SIZE]; /**< application id */
    // coverity[autosar_cpp14_a9_6_1_violation] False positive, size is well defined DLT_ID_SIZE -> 4
    char ctid[DLT_ID_SIZE]; /**< context id */
} PACKED DltExtendedHeader;
// NOLINTEND(modernize-avoid-c-arrays)

// NOLINTBEGIN(score-banned-preprocessor-directives) see below
/**
 * Suppress compiler warnings related to Wpacked and Wattributes
 * because some structures delared with PACKED attribute
 * if this attribute has no effect it causes warning
 */
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

#endif /* DLT_COMMON_H */
