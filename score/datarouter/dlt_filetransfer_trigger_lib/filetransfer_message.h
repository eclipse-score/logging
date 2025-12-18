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

#ifndef SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_FILETRANSFER_MESSAGE_H
#define SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_FILETRANSFER_MESSAGE_H

#include "score/datarouter/include/dlt/dltid.h"

#include <string>

namespace score::logging
{

struct FileTransferEntry
{
    // Suppress "AUTOSAR C++14 A9-5-1", The rule states: "Unions shall not be used."
    // Unavoidable use of of union type, in future dltit_t with LoggingIdentifier.
    // coverity[autosar_cpp14_a9_5_1_violation] see above
    score::platform::dltid_t appid{};
    // Suppress "AUTOSAR C++14 A9-5-1", The rule states: "Unions shall not be used."
    // Unavoidable use of of union type, in future dltit_t with LoggingIdentifier.
    // coverity[autosar_cpp14_a9_5_1_violation] see above
    score::platform::dltid_t ctxid{};
    std::string file_name = "";
    uint8_t delete_file = 0;
};

}  // namespace score::logging

#endif  // SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_FILETRANSFER_MESSAGE_H
