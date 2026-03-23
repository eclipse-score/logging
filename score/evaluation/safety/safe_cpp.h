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

/// @file safe_cpp.h
/// @brief Safe-C++ / defensive-programming violations for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §7 for expected review comments):
///   [SC-02] Integer overflow: signed arithmetic without overflow guard.
///   [SC-03] Narrowing conversion: storing int in char without explicit cast.
///   [SC-04] Unchecked std::vector::at() exception vs raw operator[] – incorrect usage.
///   [SC-05] Exception thrown from destructor – may terminate during stack unwinding.
///   [SC-06] Unhandled exception escaping thread function – std::terminate.
///   [SC-07] Using reinterpret_cast to read object bytes – strict-aliasing violation.
///   [SC-09] Using std::stoi without try/catch – throws on bad input.
///   [SC-10] Implicit bool-to-int conversion hiding logic error.

#ifndef SCORE_EVALUATION_SAFE_CPP_H
#define SCORE_EVALUATION_SAFE_CPP_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// Safe signed-index range check (compiler-detectable sign-compare condition removed).
// ---------------------------------------------------------------------------
inline bool IndexInRange(int index, const std::vector<int>& v)
{
    return (index >= 0) && (static_cast<std::size_t>(index) < v.size());
}

// ---------------------------------------------------------------------------
// [SC-02] Integer overflow on signed arithmetic.
// ---------------------------------------------------------------------------
inline int MultiplyUnchecked(int a, int b)
{
    return a * b;  // [SC-02] signed overflow is UB in C++; no bounds check before multiply
}

// ---------------------------------------------------------------------------
// [SC-03] Narrowing conversion.
// ---------------------------------------------------------------------------
inline char TruncateToByte(int value)
{
    char c = value;  // [SC-03] implicit narrowing; high bytes silently dropped
    return c;
}

// ---------------------------------------------------------------------------
// [SC-04] Using operator[] (no bounds check) instead of at() on purpose – but
//         the intent comments are missing; reviewer cannot distinguish deliberate
//         choice from oversight.
// ---------------------------------------------------------------------------
inline int UncheckedAccess(const std::vector<int>& v, std::size_t index)
{
    return v[index];  // [SC-04] missing bounds check / assertion; should use v.at(index)
                      //         or explicitly document why bounds check is not needed
}

// ---------------------------------------------------------------------------
// [SC-05] Exception thrown from destructor.
// ---------------------------------------------------------------------------
class ThrowingDestructor
{
public:
    ~ThrowingDestructor()
    {
        // [SC-05] Throwing from a destructor while another exception is propagating
        //         calls std::terminate().  All cleanup in destructors must be noexcept.
        if (flush_on_destroy_) Flush();  // Flush() might throw
    }

    void Flush()
    {
        throw std::runtime_error("flush failed");  // can be called from destructor
    }

private:
    bool flush_on_destroy_{true};
};

// ---------------------------------------------------------------------------
// [SC-06] Exception escaping a thread function – std::terminate.
// ---------------------------------------------------------------------------
inline void StartBadThread()
{
    std::thread t([]()
    {
        // [SC-06] Any uncaught exception in a thread calls std::terminate().
        //         Must wrap in try/catch or propagate via std::promise/future.
        throw std::runtime_error("unhandled in thread");
    });
    t.detach();
}

// ---------------------------------------------------------------------------
// [SC-07] Strict-aliasing violation via reinterpret_cast.
// ---------------------------------------------------------------------------
inline uint32_t FloatBitsUnsafe(float f)
{
    // [SC-07] Violates strict-aliasing rule; UB.  Use std::memcpy or std::bit_cast (C++20).
    return *reinterpret_cast<const uint32_t*>(&f);
}

// ---------------------------------------------------------------------------
// Sequenced argument evaluation helper (compiler-detectable sequence-point condition removed).
// ---------------------------------------------------------------------------
inline int Add(int a, int b) { return a + b; }

inline void DemonstrateUndefinedOrder()
{
    int i = 0;
    const int first = ++i;
    const int second = ++i;
    int result = Add(first, second);
    (void)result;
}

// ---------------------------------------------------------------------------
// [SC-09] std::stoi used without exception handling.
// ---------------------------------------------------------------------------
inline int ParseUnchecked(const std::string& s)
{
    return std::stoi(s);  // [SC-09] throws std::invalid_argument or std::out_of_range
                          //         with no surrounding try/catch; caller gets no chance to recover
}

// ---------------------------------------------------------------------------
// [SC-10] Implicit bool-to-int conversion masking a logic error.
// ---------------------------------------------------------------------------
inline int CountFlags(bool a, bool b, bool c)
{
    // [SC-10] Relies on implicit bool→int promotion.  While technically valid,
    //         this is error-prone and hard to read; prefer explicit static_cast<int>.
    return a + b + c;
}

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_SAFE_CPP_H
