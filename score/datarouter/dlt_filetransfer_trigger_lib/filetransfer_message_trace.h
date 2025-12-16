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

#ifndef SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_FILETRANSFER_MESSAGE_TRACE_H
#define SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_FILETRANSFER_MESSAGE_TRACE_H

#include "filetransfer_message.h"

#include "score/datarouter/include/dlt/dltid.h"
#include "score/datarouter/include/dlt/logentry_trace.h"

#include "static_reflection_with_serialization/serialization/for_logging.h"

namespace score::logging
{

MEMCPY_SERIALIZABLE(score::common::visitor::payload_tags::text, score::platform::dltid_t)
// Suppress "AUTOSAR C++14 A18-9-4". The rule states: An argument to std::forward shall not be subsequently used.
// Rationale: STRUCT_TRACEABLE is a macro, std::forward is from the macro definition
// Suppress "AUTOSAR C++14 M3-2-3". The rule states: "A type, object or function that is used in
// multiple translation units shall be declared in one and only one file.".
// Rationale: This is a forward declaration that does not vioalate this rule.
// coverity[autosar_cpp14_a18_9_4_violation]
// coverity[autosar_cpp14_m3_2_3_violation]
STRUCT_TRACEABLE(FileTransferEntry, appid, ctxid, file_name, delete_file)

}  // namespace score::logging

#endif  // SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_FILETRANSFER_MESSAGE_TRACE_H
