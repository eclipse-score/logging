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

// This file is intentionally written with MISRA C++ and CERT C++ violations
// so the CodeQL analysis workflow has something to detect on the PR diff.
// Each violation below is paired with the CodeQL query rule ID expected to
// fire for it. DO NOT use any of these patterns in production code.
//
// NOTE: C headers (<stdio.h> etc.) are used intentionally instead of C++
// headers (<cstdio> etc.). CodeQL's security library models are keyed on
// global C-linkage names (::scanf, ::strcpy, ::malloc). When <cstdio> is
// used, calls like std::scanf bind to a different Function entity in CodeQL's
// AST that the security models do not cover. Using C headers ensures every
// call resolves to the modeled global C function.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- 1. cpp/scan-vulnerable -------------------------------------------------
// Unbounded %s in scanf — classic stack buffer overflow path.
extern "C" void cwe_scan_vulnerable()
{
    char buf[8];
    scanf("%s", buf);  // CodeQL: cpp/scan-vulnerable / cpp/cwe-120
    printf("%s\n", buf);
}

// ---- 2. cpp/missing-check-against-null --------------------------------------
// malloc result dereferenced without null check.
extern "C" void cwe_null_dereference()
{
    char* buf = static_cast<char*>(malloc(16));
    buf[0] = 'A';  // CodeQL: cpp/missing-check-against-null (buf may be null)
    free(buf);
}

// ---- 3. cpp/dangerous-function (strcpy + sprintf) ---------------------------
extern "C" void cwe_dangerous_functions(const char* user_input)
{
    char small[4];
    strcpy(small, user_input);           // CodeQL: cpp/dangerous-function (strcpy)
    char fmt_buf[8];
    sprintf(fmt_buf, "%s", user_input);  // CodeQL: cpp/dangerous-function (sprintf)
    printf("%s/%s\n", small, fmt_buf);
}

// ---- 4. cpp/return-stack-allocated-memory -----------------------------------
// Returning pointer to local variable.
extern "C" const char* cwe_return_stack_address()
{
    char local[16];
    sprintf(local, "stack-data");
    return local;  // CodeQL: cpp/return-stack-allocated-memory
}

// ---- 5. cpp/uninitialized-local ---------------------------------------------
extern "C" int cwe_uninitialized_local(int seed)
{
    int x;                       // not initialized
    if (seed > 0) {
        x = seed * 2;
    }
    return x + 1;                // CodeQL: cpp/uninitialized-local (when seed <= 0)
}

// ---- 6. cpp/comparison-with-wider-type / cpp/sign-conversion ----------------
extern "C" int cwe_signed_comparison(int n)
{
    unsigned int len = (unsigned int)strlen("hello");
    if (n < len) {              // CodeQL: cpp/comparison-with-wider-type / signed-unsigned
        return 1;
    }
    return 0;
}

// ---- 7. MISRA Rule M5-2-4 / AUTOSAR A5-2-2: C-style cast --------------------
extern "C" double misra_c_style_cast(int v)
{
    return (double)v;            // MISRA: should use static_cast<double>(v)
}

// ---- 8. MISRA Rule M6-6-1 / AUTOSAR A6-6-1: goto ---------------------------
extern "C" int misra_goto(int x)
{
    if (x < 0) {
        goto err;                // MISRA: goto shall not be used
    }
    return x;
err:
    return -1;
}

// ---- 9. CERT MEM50-CPP / cpp/use-after-free --------------------------------
extern "C" int cwe_use_after_free()
{
    char* p = static_cast<char*>(malloc(16));
    if (p == nullptr) return 0;
    free(p);
    return p[0];                 // CodeQL: cpp/use-after-free
}

// ---- 10. CERT EXP34-C: pointer arithmetic out of bounds --------------------
extern "C" void cwe_out_of_bounds_write(int n)
{
    char buf[4];
    for (int i = 0; i <= n; ++i) {  // off-by-one: i can equal n
        buf[i] = static_cast<char>(i);
    }
    printf("%s\n", buf);
}

int main()
{
    cwe_scan_vulnerable();
    cwe_null_dereference();
    cwe_dangerous_functions("attacker-controlled-long-string");
    auto* dangling = cwe_return_stack_address();
    printf("%p\n", static_cast<const void*>(dangling));
    (void)cwe_uninitialized_local(-1);
    (void)cwe_signed_comparison(-5);
    (void)misra_c_style_cast(42);
    (void)misra_goto(-1);
    (void)cwe_use_after_free();
    cwe_out_of_bounds_write(10);
    return 0;
}
