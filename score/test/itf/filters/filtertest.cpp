/*******************************************************************************
 *
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
 *******************************************************************************
 */

#include "filtertest.h"

#include <Tracing>
#include <unistd.h>

#include "score/mw/log/logging.h"

struct UndefinedStruct
{
    int a;
    double b;
};

STRUCT_VISITABLE(UndefinedStruct, a, b)

int main(int, char**)
{
    score::mw::log::LogInfo() << "Filtertest starting";

    AAAA_kFatal a11{{1, 1}};
    TRACE(a11);
    AAAA_kError a12{{1, 2}};
    TRACE(a12);
    AAAA_kWarn a13{{1, 3}};
    TRACE(a13);
    AAAA_kInfo a14{{1, 4}};
    TRACE(a14);
    AAAA_kDebug a15{{1, 5}};
    TRACE(a15);
    AAAA_kVerbose a16{{1, 6}};
    TRACE(a16);

    BBBB_kFatal b11{{2, 1}};
    TRACE(b11);
    BBBB_kError b12{{2, 2}};
    TRACE(b12);
    BBBB_kWarn b13{{2, 3}};
    TRACE(b13);
    BBBB_kInfo b14{{2, 4}};
    TRACE(b14);
    BBBB_kDebug b15{{2, 5}};
    TRACE(b15);
    BBBB_kVerbose b16{{2, 6}};
    TRACE(b16);

    CCCC_kFatal c11{{3, 1}};
    TRACE(c11);
    CCCC_kError c12{{3, 2}};
    TRACE(c12);
    CCCC_kWarn c13{{3, 3}};
    TRACE(c13);
    CCCC_kInfo c14{{3, 4}};
    TRACE(c14);
    CCCC_kDebug c15{{3, 5}};
    TRACE(c15);
    CCCC_kVerbose c16{{3, 6}};
    TRACE(c16);

    UndefinedStruct us{1, 1.0};
    TRACE(us);

    sleep(1);

    score::mw::log::LogInfo() << "Filtertest done";

    return 0;
}
