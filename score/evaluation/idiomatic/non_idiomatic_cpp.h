// *******************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

/// @file non_idiomatic_cpp.h
/// @brief Non-idiomatic C++ patterns for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §3 for expected review comments):
///   [NI-01] C-style cast instead of static_cast / reinterpret_cast.
///   [NI-02] NULL macro instead of nullptr.
///   [NI-03] #define constant instead of constexpr.
///   [NI-04] C-style array instead of std::array.
///   [NI-05] printf / sprintf instead of std::to_string or streams.
///   [NI-06] Manual loop instead of std algorithm (std::accumulate, etc.).
///   [NI-07] Mutable global state / non-const global variable.
///   [NI-08] Passing std::string by value where const-ref or string_view suffices.
///   [NI-09] Using std::endl (flushes) instead of '\n'.
///   [NI-10] Out-parameters (raw pointer out-param) instead of returning value/pair/struct.

#ifndef SCORE_EVALUATION_NON_IDIOMATIC_CPP_H
#define SCORE_EVALUATION_NON_IDIOMATIC_CPP_H

#include <cstdio>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// [NI-03] #define constant – should use constexpr.
// ---------------------------------------------------------------------------
#define MAX_ITEMS 64           // [NI-03]: prefer constexpr int kMaxItems = 64;
#define PI_VALUE  3.14159265   // [NI-03]: prefer constexpr double kPi = 3.14159265;

// ---------------------------------------------------------------------------
// [NI-07] Mutable non-const global variable.
// ---------------------------------------------------------------------------
int g_request_count = 0;  // [NI-07]: global mutable state; prefer encapsulation

// ---------------------------------------------------------------------------
// [NI-01] C-style casts.
// ---------------------------------------------------------------------------
inline double ToDouble(int value)
{
    return (double)value;          // [NI-01]: use static_cast<double>(value)
}

inline void* GetRawPtr(int* p)
{
    return (void*)p;               // [NI-01]: use static_cast or reinterpret_cast with comment
}

// ---------------------------------------------------------------------------
// [NI-02] NULL instead of nullptr.
// ---------------------------------------------------------------------------
inline bool IsNull(int* ptr)
{
    return ptr == NULL;            // [NI-02]: use nullptr
}

void SetPointer(int** pp)
{
    *pp = NULL;                    // [NI-02]: use nullptr
}

// ---------------------------------------------------------------------------
// [NI-04] C-style array instead of std::array.
// ---------------------------------------------------------------------------
struct LegacyBuffer
{
    int  data[MAX_ITEMS];          // [NI-04]: prefer std::array<int, kMaxItems>
    int  size;

    void Clear()
    {
        for (int i = 0; i < MAX_ITEMS; ++i)   // [NI-04] + [NI-06]
        {
            data[i] = 0;
        }
    }
};

// ---------------------------------------------------------------------------
// [NI-05] printf / sprintf instead of C++ streams or std::to_string.
// ---------------------------------------------------------------------------
inline std::string FormatMessage(int code, const std::string& text)
{
    char buf[256];
    sprintf(buf, "Code=%d Msg=%s", code, text.c_str());  // [NI-05]: unsafe, use snprintf or streams
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// [NI-06] Manual accumulation loop – should use std::accumulate.
// ---------------------------------------------------------------------------
inline int SumVector(const std::vector<int>& vec)
{
    int total = 0;
    for (int i = 0; i < (int)vec.size(); ++i)   // [NI-06] + [NI-01] C-style cast
    {
        total += vec[i];
    }
    return total;
}

// ---------------------------------------------------------------------------
// [NI-08] std::string passed by value – should be const std::string& or std::string_view.
// ---------------------------------------------------------------------------
inline bool HasPrefix(std::string text, std::string prefix)  // [NI-08]: unnecessary copy
{
    return text.substr(0, prefix.size()) == prefix;
}

// ---------------------------------------------------------------------------
// [NI-09] std::endl instead of '\n' – unnecessary flush.
// ---------------------------------------------------------------------------
inline void PrintLines(const std::vector<std::string>& lines)
{
    for (const auto& line : lines)
    {
        std::cout << line << std::endl;  // [NI-09]: prefer '\n'; endl flushes the buffer
    }
}

// ---------------------------------------------------------------------------
// [NI-10] Out-parameter instead of returning a value.
// ---------------------------------------------------------------------------
inline bool ParseInt(const std::string& str, int* result)   // [NI-10]: out-param anti-pattern
{
    if (str.empty() || result == NULL)  // [NI-02] reuse
    {
        return false;
    }
    *result = std::stoi(str);
    return true;
}

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_NON_IDIOMATIC_CPP_H
