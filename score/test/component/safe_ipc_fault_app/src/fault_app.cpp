/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

#include "score/mw/log/logger.h"

#include <iostream>

int main()
{
    score::mw::log::Logger& g_logger_ctx0{score::mw::log::CreateLogger("LOGG", "some context of application LSIS")};
    std::cout << "\n Hello There \n " << std::endl;
    (void)g_logger_ctx0;
    return 0;
}
