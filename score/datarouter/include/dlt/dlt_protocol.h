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
 * $LastChangedRevision$
 * $LastChangedDate$
 * $LastChangedBy$
 Initials    Date         Comment
 aw          13.01.2010   initial
 */
#ifndef DLT_PROTOCOL_H
#define DLT_PROTOCOL_H
#include "dlt/dlt_common.h"
/**
  \defgroup protocolapi DLT Protocol API
  \addtogroup protocolapi
  \{
*/

#include "score/os/utils/path.h"

#include "score/span.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <optional>

/*
 * Definitions of the htyp parameter in standard header.
 */
#define DLT_HTYP_UEH 0x01  /**< use extended header */
#define DLT_HTYP_MSBF 0x02 /**< MSB first */
#define DLT_HTYP_WEID 0x04 /**< with ECU ID */
#define DLT_HTYP_WSID 0x08 /**< with session ID */
#define DLT_HTYP_WTMS 0x10 /**< with timestamp */
#define DLT_HTYP_VERS 0x20 /**< version number, 0x1 */

#define DLT_IS_HTYP_UEH(htyp) ((htyp) & DLT_HTYP_UEH)
#define DLT_IS_HTYP_MSBF(htyp) ((htyp) & DLT_HTYP_MSBF)
#define DLT_IS_HTYP_WEID(htyp) ((htyp) & DLT_HTYP_WEID)
#define DLT_IS_HTYP_WSID(htyp) ((htyp) & DLT_HTYP_WSID)
#define DLT_IS_HTYP_WTMS(htyp) ((htyp) & DLT_HTYP_WTMS)

#define DLT_HTYP_PROTOCOL_VERSION1 (1 << 5)

/*
 * Definitions of msin parameter in extended header.
 */
#define DLT_MSIN_VERB 0x01 /**< verbose */
#define DLT_MSIN_MSTP 0x0e /**< message type */
#define DLT_MSIN_MTIN 0xf0 /**< message type info */

#define DLT_MSIN_MSTP_SHIFT 1 /**< shift right offset to get mstp value */
#define DLT_MSIN_MTIN_SHIFT 4 /**< shift right offset to get mtin value */

#define DLT_IS_MSIN_VERB(msin) ((msin) & DLT_MSIN_VERB)
#define DLT_GET_MSIN_MSTP(msin) (((msin) & DLT_MSIN_MSTP) >> DLT_MSIN_MSTP_SHIFT)
#define DLT_GET_MSIN_MTIN(msin) (((msin) & DLT_MSIN_MTIN) >> DLT_MSIN_MTIN_SHIFT)

/*
 * Definitions of mstp parameter in extended header.
 */
#define DLT_TYPE_LOG 0x00       /**< Log message type */
#define DLT_TYPE_APP_TRACE 0x01 /**< Application trace message type */
#define DLT_TYPE_NW_TRACE 0x02  /**< Network trace message type */
#define DLT_TYPE_CONTROL 0x03   /**< Control message type */

/*
 * Definitions of msti parameter in extended header.
 */
#define DLT_TRACE_VARIABLE 0x01     /**< tracing of a variable */
#define DLT_TRACE_FUNCTION_IN 0x02  /**< tracing of function calls */
#define DLT_TRACE_FUNCTION_OUT 0x03 /**< tracing of function return values */
#define DLT_TRACE_STATE 0x04        /**< tracing of states of a state machine */
#define DLT_TRACE_VFB 0x05          /**< tracing of virtual function bus */

/*
 * Definitions of msbi parameter in extended header.
 */

/* see file dlt_user.h */

/*
 * Definitions of msci parameter in extended header.
 */
#define DLT_CONTROL_REQUEST 0x01  /**< Request message */
#define DLT_CONTROL_RESPONSE 0x02 /**< Response to request message */
#define DLT_CONTROL_TIME 0x03     /**< keep-alive message */

#define DLT_MSIN_CONTROL_REQUEST \
    ((DLT_TYPE_CONTROL << DLT_MSIN_MSTP_SHIFT) | (DLT_CONTROL_REQUEST << DLT_MSIN_MTIN_SHIFT))
#define DLT_MSIN_CONTROL_RESPONSE \
    ((DLT_TYPE_CONTROL << DLT_MSIN_MSTP_SHIFT) | (DLT_CONTROL_RESPONSE << DLT_MSIN_MTIN_SHIFT))
#define DLT_MSIN_CONTROL_TIME ((DLT_TYPE_CONTROL << DLT_MSIN_MSTP_SHIFT) | (DLT_CONTROL_TIME << DLT_MSIN_MTIN_SHIFT))

/*
 * Definitions of types of arguments in payload.
 */
#define DLT_TYPE_INFO_TYLE \
    0x0000000f /**< Length of standard data: 1 = 8bit, 2 = 16bit, 3 = 32 bit, 4 = 64 bit, 5 = 128 bit */
#define DLT_TYPE_INFO_BOOL 0x00000010U /**< Boolean data */
#define DLT_TYPE_INFO_SINT 0x00000020U /**< Signed integer data */
#define DLT_TYPE_INFO_UINT 0x00000040U /**< Unsigned integer data */
#define DLT_TYPE_INFO_FLOA 0x00000080U /**< Float data */
#define DLT_TYPE_INFO_ARAY 0x00000100U /**< Array of standard types */
#define DLT_TYPE_INFO_STRG 0x00000200U /**< String */
#define DLT_TYPE_INFO_RAWD 0x00000400U /**< Raw data */
#define DLT_TYPE_INFO_VARI 0x00000800U /**< Set, if additional information to a variable is available */
#define DLT_TYPE_INFO_FIXP 0x00001000U /**< Set, if quantization and offset are added */
#define DLT_TYPE_INFO_TRAI 0x00002000U /**< Set, if additional trace information is added */
#define DLT_TYPE_INFO_STRU 0x00004000U /**< Struct */
#define DLT_TYPE_INFO_SCOD 0x00038000U /**< coding of the type string: 0 = ASCII, 1 = UTF-8 */

#define DLT_TYLE_8BIT 0x00000001U
#define DLT_TYLE_16BIT 0x00000002U
#define DLT_TYLE_32BIT 0x00000003U
#define DLT_TYLE_64BIT 0x00000004U
#define DLT_TYLE_128BIT 0x00000005U

#define DLT_SCOD_ASCII 0x00000000U
#define DLT_SCOD_UTF8 0x00008000U
#define DLT_SCOD_HEX 0x00010000U
#define DLT_SCOD_BIN 0x00018000U

/*
 * Definitions of DLT services.
 */
#define DLT_SERVICE_ID_SET_LOG_LEVEL 0x01            /**< Service ID: Set log level */
#define DLT_SERVICE_ID_SET_TRACE_STATUS 0x02         /**< Service ID: Set trace status */
#define DLT_SERVICE_ID_GET_LOG_INFO 0x03             /**< Service ID: Get log info */
#define DLT_SERVICE_ID_GET_DEFAULT_LOG_LEVEL 0x04    /**< Service ID: Get dafault log level */
#define DLT_SERVICE_ID_STORE_CONFIG 0x05             /**< Service ID: Store configuration */
#define DLT_SERVICE_ID_RESET_TO_FACTORY_DEFAULT 0x06 /**< Service ID: Reset to factory defaults */
#define DLT_SERVICE_ID_SET_COM_INTERFACE_STATUS 0x07 /**< Service ID: Set communication interface status */
#define DLT_SERVICE_ID_SET_COM_INTERFACE_MAX_BANDWIDTH \
    0x08                                             /**< Service ID: Set communication interface maximum bandwidth */
#define DLT_SERVICE_ID_SET_VERBOSE_MODE 0x09         /**< Service ID: Set verbose mode */
#define DLT_SERVICE_ID_SET_MESSAGE_FILTERING 0x0A    /**< Service ID: Set message filtering */
#define DLT_SERVICE_ID_SET_TIMING_PACKETS 0x0B       /**< Service ID: Set timing packets */
#define DLT_SERVICE_ID_GET_LOCAL_TIME 0x0C           /**< Service ID: Get local time */
#define DLT_SERVICE_ID_USE_ECU_ID 0x0D               /**< Service ID: Use ECU id */
#define DLT_SERVICE_ID_USE_SESSION_ID 0x0E           /**< Service ID: Use session id */
#define DLT_SERVICE_ID_USE_TIMESTAMP 0x0F            /**< Service ID: Use timestamp */
#define DLT_SERVICE_ID_USE_EXTENDED_HEADER 0x10      /**< Service ID: Use extended header */
#define DLT_SERVICE_ID_SET_DEFAULT_LOG_LEVEL 0x11    /**< Service ID: Set default log level */
#define DLT_SERVICE_ID_SET_DEFAULT_TRACE_STATUS 0x12 /**< Service ID: Set default trace status */
#define DLT_SERVICE_ID_GET_SOFTWARE_VERSION 0x13     /**< Service ID: Get software version */
#define DLT_SERVICE_ID_MESSAGE_BUFFER_OVERFLOW 0x14  /**< Service ID: Message buffer overflow */
#define DLT_SERVICE_ID_LAST_ENTRY \
    0x15 /**< Service ID: Last entry to avoid any further modifications in dependent code */
#define DLT_SERVICE_ID_UNREGISTER_CONTEXT 0xf01             /**< Service ID: Message unregister context */
#define DLT_SERVICE_ID_CONNECTION_INFO 0xf02                /**< Service ID: Message connection info */
#define DLT_SERVICE_ID_TIMEZONE 0xf03                       /**< Service ID: Timezone */
#define DLT_SERVICE_ID_MARKER 0xf04                         /**< Service ID: Marker */
#define DLT_SERVICE_ID_OFFLINE_LOGSTORAGE 0xf05             /**< Service ID: Offline log storage */
#define DLT_SERVICE_ID_PASSIVE_NODE_CONNECT 0xf0E           /**< Service ID: (Dis)Connect passive Node */
#define DLT_SERVICE_ID_PASSIVE_NODE_CONNECTION_STATUS 0xf0F /**< Service ID: Passive Node status information */
#define DLT_SERVICE_ID_SET_ALL_LOG_LEVEL 0xf10              /**< Service ID: set all log level */
#define DLT_SERVICE_ID_CALLSW_CINJECTION 0xFFF              /**< Service ID: Message Injection (minimal ID) */

/*
 * Definitions of DLT service response status
 */
#define DLT_SERVICE_RESPONSE_OK 0x00            /**< Control message response: OK */
#define DLT_SERVICE_RESPONSE_NOT_SUPPORTED 0x01 /**< Control message response: Not supported */
#define DLT_SERVICE_RESPONSE_ERROR 0x02         /**< Control message response: Error */

/*
 * Definitions of DLT service connection state
 */
#define DLT_CONNECTION_STATUS_DISCONNECTED 0x01 /**< Client is disconnected */
#define DLT_CONNECTION_STATUS_CONNECTED 0x02    /**< Client is connected */

/**
  \}
*/
static constexpr uint8_t FLST_NOR = 8;
static constexpr uint8_t FLDA_NOR = 5;
static constexpr uint8_t FLFI_NOR = 3;
static constexpr uint8_t FLIF_NOR = 7;
static constexpr uint8_t FLER_FILE_NOR = 9;
static constexpr uint8_t FLER_NO_FILE_NOR = 5;

//! Defines the buffer size of a single file package which will be logged to dlt
static constexpr uint16_t BUFFER_SIZE = 1024;
//! Defines the data package size  which will be logged to dlt
static constexpr uint16_t PKG_FLAG_BYTES = 2 + 5 + 4;
static constexpr uint16_t PKG_SERIALNO_BYTES = 4 + 4;
static constexpr uint16_t PKG_PKGNO_BYTES = 4 + 4;
static constexpr uint16_t PKG_RAWDATA_BYTES = 4 + 2 + BUFFER_SIZE;
static constexpr uint16_t DATAPKGSIZE =
    PKG_FLAG_BYTES + PKG_SERIALNO_BYTES + PKG_PKGNO_BYTES + PKG_RAWDATA_BYTES + PKG_FLAG_BYTES;
/* ! Error code for dlt_user_log_file_complete */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_COMPLETE = -300;
/* ! Error code for dlt_user_log_file_complete */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_COMPLETE1 = -301;
/* ! Error code for dlt_user_log_file_complete */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_COMPLETE2 = -302;
/* ! Error code for dlt_user_log_file_complete */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_COMPLETE3 = -303;
/* ! Error code for dlt_user_log_file_head */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_HEAD = -400;
/* ! Error code for dlt_user_log_file_data */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_DATA = -500;
/* ! Error code for dlt_user_log_file_data */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_DATA_USER_BUFFER_FAILED = -501;
/* ! Error code for dlt_user_log_file_end */
static constexpr int16_t DLT_FILETRANSFER_ERROR_FILE_END = -600;
/* ! Error code for dlt_user_log_file_infoAbout */
static constexpr int16_t DLT_FILETRANSFER_ERROR_INFO_ABOUT = -700;
/* ! Error code for dlt_user_log_file_packagesCount */
static constexpr int16_t DLT_FILETRANSFER_ERROR_PACKAGE_COUNT = -800;

inline std::optional<std::tuple<score::cpp::span<char> /*buffer*/, uint8_t /*args count*/>> PackageFileHeader(
    score::cpp::span<char> data_span,
    const uint32_t serialno,
    const std::string& filename,
    const uint32_t fsize,
    const std::string& creationdate,
    const uint32_t packagecount)
{
    // NULL terminated "FLST" string, used by raw memory access
    const std::array<char, 5> transfer_type = {'F', 'L', 'S', 'T', 0};
    const auto string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    uint32_t type_info = 0;
    const std::string alias = score::os::Path::instance().get_base_name(filename);

    if (const auto buffersize_limit =
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(string_length_data_1)) +
            static_cast<uint32_t>(sizeof(transfer_type)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(serialno)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(string_length_data_1)) + static_cast<uint32_t>(alias.size() + 1) +
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(fsize)) +
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(string_length_data_1)) +
            static_cast<uint32_t>(creationdate.size() + 1) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(packagecount)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(uint16_t)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(string_length_data_1)) + static_cast<uint32_t>(sizeof(transfer_type));
        buffersize_limit > data_span.size())
    {
        return std::nullopt;
    }

    // NOLINTBEGIN(score-banned-function) see below
    // memcpy used to construct file header in raw memory block
    // there is no special type available (to access it by member)
    // file transfer logic constructs packets in raw memory
    // so memcpy required to build this packets
    uint32_t buffersize = 0;
    char* buffer = data_span.data();
    // package flag "FLST"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));

    // serialno
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &serialno, sizeof(serialno));
    buffersize += static_cast<uint32_t>(sizeof(serialno));

    // file name
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    const auto string_alias_length_data_1 = static_cast<uint16_t>(alias.size() + 1);
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_alias_length_data_1, sizeof(string_alias_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_alias_length_data_1));
    std::memcpy(std::next(buffer, buffersize), alias.data(), alias.size());
    buffersize += static_cast<uint32_t>(alias.size() + 1);

    // file size
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &fsize, sizeof(fsize));
    buffersize += static_cast<uint32_t>(sizeof(fsize));

    // creation date
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    const auto string_length_creation = static_cast<uint16_t>(creationdate.size() + 1);
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_creation, sizeof(string_length_creation));
    buffersize += static_cast<uint32_t>(sizeof(string_length_creation));
    std::memcpy(std::next(buffer, buffersize), creationdate.data(), creationdate.size());
    buffersize += static_cast<uint32_t>(creationdate.size() + 1);

    // packagecount
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &packagecount, sizeof(packagecount));
    buffersize += static_cast<uint32_t>(sizeof(packagecount));

    // buffer size
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_16BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &BUFFER_SIZE, sizeof(uint16_t));
    buffersize += static_cast<uint32_t>(sizeof(uint16_t));

    // package flag "FLST"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));
    // NOLINTEND(score-banned-function)

    return std::tuple{data_span.subspan(0, buffersize), FLST_NOR};
}

inline std::optional<std::tuple<score::cpp::span<char> /*buffer*/, uint8_t /*args count*/>>
PackageFileData(score::cpp::span<char> buffer_in, FILE* file, const uint32_t serialno, const uint32_t pkgno)
{
    char* buffer = buffer_in.data();
    uint32_t buffersize{0};
    // NULL terminated "FLDA" string, used by raw memory access
    const std::array<char, 5> transfer_type = {'F', 'L', 'D', 'A', 0};
    uint16_t string_length_data_1 = 0;
    uint32_t type_info = 0;
    uint32_t readbytes = 0;
    // To write a test that gets inside the if condition we should:
    // - Provide negative value for "pkgno" which is not possible because its datatype is unsigned and not signed.
    // - Work with already closed file. A unit test already implemented but it sporadically raises a memory leak.
    // - Providing a value for the whence argument that is not SEEK_SET, SEEK_CUR, or SEEK_END(it's fixed to SEEK_SET).
    // So, safer to suppress it.
    if (0 != fseek(file, (pkgno - 1) * BUFFER_SIZE, SEEK_SET))  // LCOV_EXCL_BR_LINE
    {
        //  File access error:
        return std::nullopt;  // LCOV_EXCL_LINE
    }

    if (const auto buffersize_max =
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(string_length_data_1)) +
            static_cast<uint32_t>(sizeof(transfer_type)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(serialno)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(pkgno)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(uint16_t)) + BUFFER_SIZE + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(string_length_data_1)) + static_cast<uint32_t>(sizeof(transfer_type));
        buffersize_max > buffer_in.size())
    {
        return std::nullopt;
    }

    // NOLINTBEGIN(score-banned-function) see below
    // memcpy used to copy raw file data to memory block
    // file transfer logic constructs packets in raw memory
    // so memcpy required to build this packets
    // package flag "FLDA"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));

    // serialno
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &serialno, sizeof(serialno));
    buffersize += static_cast<uint32_t>(sizeof(serialno));

    // packageno
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &pkgno, sizeof(pkgno));
    buffersize += static_cast<uint32_t>(sizeof(pkgno));

    // Raw Data
    type_info = DLT_TYPE_INFO_RAWD;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    readbytes = static_cast<uint32_t>(fread(
        std::next(buffer, buffersize + static_cast<uint32_t>(sizeof(uint16_t))), sizeof(char), BUFFER_SIZE, file));
    std::memcpy(std::next(buffer, buffersize), &readbytes, sizeof(uint16_t));
    buffersize += static_cast<uint32_t>(sizeof(uint16_t));
    buffersize += readbytes;

    // package flag "FLDA"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));
    // NOLINTEND(score-banned-function)

    return std::tuple{buffer_in.subspan(0, buffersize), FLDA_NOR};
}

inline std::optional<std::tuple<score::cpp::span<char> /*buffersize*/, uint8_t /*args count*/>> PackageFileEnd(
    score::cpp::span<char> data_buffer,
    const uint32_t serialno)
{
    // NULL terminated "FLFI" string, used by raw memory access
    const std::array<char, 5> transfer_type = {'F', 'L', 'F', 'I', 0};
    uint16_t string_length_data_1 = 0;
    uint32_t type_info = 0;

    if (const auto buffer_limit_size =
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(string_length_data_1)) +
            static_cast<uint32_t>(sizeof(transfer_type)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(serialno)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(string_length_data_1)) + static_cast<uint32_t>(sizeof(transfer_type));
        buffer_limit_size > data_buffer.size())
    {
        return std::nullopt;
    }

    uint32_t buffersize = 0;
    char* buffer = data_buffer.data();

    // NOLINTBEGIN(score-banned-function) see below
    // memcpy used to construct FileEnd packet in raw memory
    // file transfer logic constructs packets in raw memory
    // so memcpy required to build this packets
    // package flag "FLFI"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));

    // serialno
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &serialno, sizeof(serialno));
    buffersize += static_cast<uint32_t>(sizeof(serialno));

    // package flag "FLFI"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));
    // NOLINTEND(score-banned-function)

    return std::tuple(data_buffer.first(buffersize), FLFI_NOR);
}

inline std::optional<std::tuple<score::cpp::span<char> /*buffer*/, uint8_t /*args count*/>> PackageFileError(
    score::cpp::span<char> data_span,
    const int16_t errorcode,
    const uint32_t serialno,
    const std::string& filename,
    const uint32_t fsize,
    const std::string& creationdate,
    const uint32_t packagecount,
    const std::string& error_msg = "")
{
    // NULL terminated "FLER" string, used by raw memory access
    const std::array<char, 5> transfer_type = {'F', 'L', 'E', 'R', 0};
    const auto string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    uint32_t type_info = 0;
    const std::string alias = score::os::Path::instance().get_base_name(filename);
    const auto alias_string_length = static_cast<uint16_t>(alias.size() + 1);
    uint32_t buffersize = 0;
    char* buffer = data_span.data();
    uint8_t nor = FLER_FILE_NOR;

    // NOLINTBEGIN(score-banned-function) see below
    // memcpy used to copy construct FileEnd packet in raw memory
    // file transfer logic constructs packets in raw memory
    // so memcpy required to build this packets
    if (errno != ENOENT)
    {
        // package flag "FLER"
        type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
        buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
        std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
        buffersize += static_cast<uint32_t>(sizeof(transfer_type));

        type_info = DLT_TYPE_INFO_SINT | DLT_TYLE_16BIT;
        // error code
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &errorcode, sizeof(errorcode));
        buffersize += static_cast<uint32_t>(sizeof(int16_t));

        // linux error code
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &errno, sizeof(errno));
        buffersize += static_cast<uint32_t>(sizeof(int16_t));

        // serialno
        type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &serialno, sizeof(serialno));
        buffersize += static_cast<uint32_t>(sizeof(serialno));

        // file name
        type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &alias_string_length, sizeof(alias_string_length));
        buffersize += static_cast<uint32_t>(sizeof(alias_string_length));
        std::memcpy(std::next(buffer, buffersize), alias.data(), alias.size());
        buffersize += static_cast<uint32_t>(alias.size() + 1);

        // file size
        type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &fsize, sizeof(fsize));
        buffersize += static_cast<uint32_t>(sizeof(fsize));

        // creation date
        type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
        const auto creationdate_string_length = static_cast<uint16_t>(creationdate.size() + 1);
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &creationdate_string_length, sizeof(creationdate_string_length));
        buffersize += static_cast<uint32_t>(sizeof(creationdate_string_length));
        std::memcpy(std::next(buffer, buffersize), creationdate.data(), creationdate.size());
        buffersize += static_cast<uint32_t>(creationdate.size() + 1);

        // packagecount
        type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &packagecount, sizeof(packagecount));
        buffersize += static_cast<uint32_t>(sizeof(packagecount));

        if (error_msg != "")
        {
            type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
            const auto string_length_err = static_cast<uint16_t>(error_msg.size() + 1);
            std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
            buffersize += static_cast<uint32_t>(sizeof(type_info));
            std::memcpy(std::next(buffer, buffersize), &string_length_err, sizeof(string_length_err));
            buffersize += static_cast<uint32_t>(sizeof(string_length_err));
            std::memcpy(std::next(buffer, buffersize), error_msg.data(), error_msg.size());
            buffersize += static_cast<uint32_t>(error_msg.size() + 1);
            nor++;
        }

        // package flag "FLER"
        type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
        buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
        std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
        buffersize += static_cast<uint32_t>(sizeof(transfer_type));
    }
    else
    {
        // package flag "FLER"
        type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
        buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
        std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
        buffersize += static_cast<uint32_t>(sizeof(transfer_type));

        type_info = DLT_TYPE_INFO_SINT | DLT_TYLE_16BIT;
        // error code
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &errorcode, sizeof(errorcode));
        buffersize += static_cast<uint32_t>(sizeof(int16_t));

        // linux error code
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &errno, sizeof(errno));
        buffersize += static_cast<uint32_t>(sizeof(int16_t));

        // file name
        type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &alias_string_length, sizeof(alias_string_length));
        buffersize += static_cast<uint32_t>(sizeof(alias_string_length));
        std::memcpy(std::next(buffer, buffersize), alias.data(), alias.size());
        buffersize += static_cast<uint32_t>(alias.size() + 1);

        // package flag "FLER"
        type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
        std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
        buffersize += static_cast<uint32_t>(sizeof(type_info));
        std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
        buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
        std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
        buffersize += static_cast<uint32_t>(sizeof(transfer_type));

        nor = FLER_NO_FILE_NOR;
    }
    // NOLINTEND(score-banned-function)
    return std::tuple{data_span.subspan(0, buffersize), nor};
}

inline std::optional<std::tuple<score::cpp::span<char> /*buffer*/, uint8_t /*args count*/>> PackageFileInformation(
    score::cpp::span<char> data_span,
    const uint32_t serialno,
    const std::string& filename,
    const uint32_t fsize,
    const std::string& creationdate,
    const uint32_t packagecount)
{
    // NULL terminated "FLIF" string, used by raw memory access
    const std::array<char, 5> transfer_type = {'F', 'L', 'I', 'F', 0};
    uint16_t string_length_data_1 = 0;
    uint32_t type_info = 0;
    const std::string alias = score::os::Path::instance().get_base_name(filename);

    if (const auto buffersize_max =
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(string_length_data_1)) +
            static_cast<uint32_t>(sizeof(transfer_type)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(serialno)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(string_length_data_1)) + static_cast<uint32_t>(alias.size() + 1) +
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(fsize)) +
            static_cast<uint32_t>(sizeof(type_info)) + static_cast<uint32_t>(sizeof(string_length_data_1)) +
            static_cast<uint32_t>(creationdate.size() + 1) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(packagecount)) + static_cast<uint32_t>(sizeof(type_info)) +
            static_cast<uint32_t>(sizeof(string_length_data_1)) + static_cast<uint32_t>(sizeof(transfer_type));
        buffersize_max > data_span.size())
    {
        return std::nullopt;
    }

    uint32_t buffersize{0};
    char* buffer = data_span.data();

    // NOLINTBEGIN(score-banned-function) see below
    // memcpy used to copy construct packet with file information in raw memory
    // file transfer logic constructs packets in raw memory
    // so memcpy required to build this packets
    // package flag "FLIF"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));

    // serialno
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &serialno, sizeof(serialno));
    buffersize += static_cast<uint32_t>(sizeof(serialno));

    // file name
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(alias.size() + 1);
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), alias.data(), alias.size());
    buffersize += static_cast<uint32_t>(alias.size() + 1);

    // file size
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &fsize, sizeof(fsize));
    buffersize += static_cast<uint32_t>(sizeof(fsize));

    // creation date
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(creationdate.size() + 1);
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), creationdate.data(), creationdate.size());
    buffersize += static_cast<uint32_t>(creationdate.size() + 1);

    // packagecount
    type_info = DLT_TYPE_INFO_UINT | DLT_TYLE_32BIT;
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &packagecount, sizeof(packagecount));
    buffersize += static_cast<uint32_t>(sizeof(packagecount));

    // package flag "FLIF"
    type_info = DLT_TYPE_INFO_STRG | DLT_SCOD_UTF8;
    string_length_data_1 = static_cast<uint16_t>(sizeof(transfer_type));
    std::memcpy(std::next(buffer, buffersize), &(type_info), sizeof(type_info));
    buffersize += static_cast<uint32_t>(sizeof(type_info));
    std::memcpy(std::next(buffer, buffersize), &string_length_data_1, sizeof(string_length_data_1));
    buffersize += static_cast<uint32_t>(sizeof(string_length_data_1));
    std::memcpy(std::next(buffer, buffersize), &transfer_type, sizeof(transfer_type));
    buffersize += static_cast<uint32_t>(sizeof(transfer_type));
    // NOLINTEND(score-banned-function)

    return std::tuple{data_span.subspan(0, buffersize), FLIF_NOR};
}

#endif /* DLT_PROTOCOL_H */
