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
#ifndef FILTERTEST_H
#define FILTERTEST_H

#include "static_reflection_with_serialization/visitor/visit_as_struct.h"

struct default_struct
{
    int a;
    char b;
};

STRUCT_VISITABLE(default_struct, a, b)

#define dummy_struct_name(ctx, level) dummy_struct_name2(ctx, level)
#define dummy_struct_name2(ctx, level) ctx##_##level

#define dummy_struct(name)  \
    struct name             \
    {                       \
        default_struct val; \
    };                      \
    STRUCT_VISITABLE(name, val)

dummy_struct(dummy_struct_name(AAAA, kFatal));
dummy_struct(dummy_struct_name(AAAA, kError));
dummy_struct(dummy_struct_name(AAAA, kWarn));
dummy_struct(dummy_struct_name(AAAA, kInfo));
dummy_struct(dummy_struct_name(AAAA, kDebug));
dummy_struct(dummy_struct_name(AAAA, kVerbose));

dummy_struct(dummy_struct_name(BBBB, kFatal));
dummy_struct(dummy_struct_name(BBBB, kError));
dummy_struct(dummy_struct_name(BBBB, kWarn));
dummy_struct(dummy_struct_name(BBBB, kInfo));
dummy_struct(dummy_struct_name(BBBB, kDebug));
dummy_struct(dummy_struct_name(BBBB, kVerbose));

dummy_struct(dummy_struct_name(CCCC, kFatal));
dummy_struct(dummy_struct_name(CCCC, kError));
dummy_struct(dummy_struct_name(CCCC, kWarn));
dummy_struct(dummy_struct_name(CCCC, kInfo));
dummy_struct(dummy_struct_name(CCCC, kDebug));
dummy_struct(dummy_struct_name(CCCC, kVerbose));

#endif  // FILTERTEST_H
