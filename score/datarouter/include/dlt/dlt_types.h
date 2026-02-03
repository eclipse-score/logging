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

#ifndef SCORE_DATAROUTER_INCLUDE_DLT_DLT_TYPES_H
#define SCORE_DATAROUTER_INCLUDE_DLT_DLT_TYPES_H

#ifdef _MSC_VER
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8 int8_t;

typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;

typedef int pid_t;
typedef unsigned int speed_t;

#define UINT16_MAX 0xFFFF

#include <varargs.h>
#else
#include <stdint.h>
#endif

/**
 * Definitions of DLT return values
 */
typedef enum
{
    kDltReturnLoggingDisabled = -7,
    kDltReturnUserBufferFull = -6,
    kDltReturnWrongParameter = -5,
    kDltReturnBufferFull = -4,
    kDltReturnPipeFull = -3,
    kDltReturnPipeError = -2,
    kDltReturnError = -1,
    kDltReturnOk = 0,
    kDltReturnTrue = 1
} DltReturnValue;

/**
 * Definitions of DLT log level
 */
typedef enum
{
    kDltLogDefault = -1,   /**< Default log level */
    kDltLogOff = 0x00,     /**< Log level off */
    kDltLogFatal = 0x01,   /**< fatal system error */
    kDltLogError = 0x02,   /**< error with impact to correct functionality */
    kDltLogWarn = 0x03,    /**< warning, correct behaviour could not be ensured */
    kDltLogInfo = 0x04,    /**< informational */
    kDltLogDebug = 0x05,   /**< debug  */
    kDltLogVerbose = 0x06, /**< highest grade of information */
    kDltLogMax = 7         /**< maximum value, used for range check */
} DltLogLevelType;

/**
 * Definitions of DLT Format
 */
typedef enum
{
    kDltFormatDefault = 0x00, /**< no sepecial format */
    kDltFormatHeX8 = 0x01,    /**< Hex 8 */
    kDltFormatHeX16 = 0x02,   /**< Hex 16 */
    kDltFormatHeX32 = 0x03,   /**< Hex 32 */
    kDltFormatHeX64 = 0x04,   /**< Hex 64 */
    kDltFormatBiN8 = 0x05,    /**< Binary 8 */
    kDltFormatBiN16 = 0x06,   /**< Binary 16  */
    kDltFormatMax = 7         /**< maximum value, used for range check */
} DltFormatType;

/**
 * Definitions of DLT trace status
 */
typedef enum
{
    kDltTraceStatusDefault = -1, /**< Default trace status */
    kDltTraceStatusOff = 0x00,   /**< Trace status: Off */
    kDltTraceStatusOn = 0x01,    /**< Trace status: On */
    kDltTraceStatusMax = 2       /**< maximum value, used for range check */
} DltTraceStatusType;

/**
 * Definitions for  dlt_user_trace_network/DLT_TRACE_NETWORK()
 * as defined in the DLT protocol
 */
typedef enum
{
    kDltNwTraceIpc = 0x01,     /**< Interprocess communication */
    kDltNwTraceCan = 0x02,     /**< Controller Area Network Bus */
    kDltNwTraceFlexray = 0x03, /**< Flexray Bus */
    kDltNwTraceMost = 0x04,    /**< Media Oriented System Transport Bus */
    kDltNwTraceReserveD0 = 0x05,
    kDltNwTraceReserveD1 = 0x06,
    kDltNwTraceReserveD2 = 0x07,
    kDltNwTraceUserDefineD0 = 0x08,
    kDltNwTraceUserDefineD1 = 0x09,
    kDltNwTraceUserDefineD2 = 0x0A,
    kDltNwTraceUserDefineD3 = 0x0B,
    kDltNwTraceUserDefineD4 = 0x0C,
    kDltNwTraceUserDefineD5 = 0x0D,
    kDltNwTraceUserDefineD6 = 0x0E,
    kDltNwTraceUserDefineD7 = 0x0F,
    kDltNwTraceMax = 16 /**< maximum value, used for range check */
} DltNetworkTraceType;

/**
 * This are the log modes.
 */
typedef enum
{
    kDltUserModeUndefined = -1,
    kDltUserModeOff = 0,
    kDltUserModeExternal = 1,
    kDltUserModeInternal = 2,
    kDltUserModeBoth = 3,
    kDltUserModeMax = 4 /**< maximum value, used for range check */
} DltUserLogMode;

typedef float Float32T;
typedef double Float64T;

#endif  // SCORE_DATAROUTER_INCLUDE_DLT_DLT_TYPES_H
