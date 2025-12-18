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

#ifndef SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_MOCK_FILETRANSFER_MOCK_H
#define SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_MOCK_FILETRANSFER_MOCK_H

#include "score/datarouter/dlt_filetransfer_trigger_lib/include/ifile_transfer.h"

#include <gmock/gmock.h>

namespace score
{
namespace logging
{

class MockFileTransfer : public IFileTransfer
{
  public:
    MOCK_METHOD(void, TransferFile, (const std::string& fname, const bool delfile), (const, override));
};

}  // namespace logging
}  // namespace score

#endif  // SCORE_PAS_LOGGING_DLT_FILETRANSFER_TRIGGER_LIB_MOCK_FILETRANSFER_MOCK_H
