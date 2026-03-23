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

/// @file exception_error_handling.h
/// @brief Exception and error-handling issues for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §10 for expected review comments):
///   [EH-02] Swallowing exceptions silently in a catch-all block.
///   [EH-03] Throwing std::string / int instead of a proper exception type.
///   [EH-04] Using exception for normal control flow (not an error condition).
///   [EH-05] noexcept function that calls a function which can throw.
///   [EH-06] Re-throwing with 'throw ex' (slices the exception) vs 'throw'.
///   [EH-07] Error code returned as int – easy for caller to ignore silently.
///   [EH-08] Constructor that silently swallows failure – leaves object half-initialised.

#ifndef SCORE_EVALUATION_EXCEPTION_ERROR_HANDLING_H
#define SCORE_EVALUATION_EXCEPTION_ERROR_HANDLING_H

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// Catching by reference (compiler-detectable catch-by-value condition removed).
// ---------------------------------------------------------------------------
inline void CatchByValue()
{
    try
    {
        throw std::runtime_error("something went wrong");
    }
    catch (const std::exception& ex)
    {
        (void)ex.what();
    }
}

// ---------------------------------------------------------------------------
// [EH-02] Silent catch-all swallows exceptions without logging or re-throw.
// ---------------------------------------------------------------------------
inline bool LoadFile(const std::string& path)
{
    try
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("cannot open");
        return true;
    }
    catch (...)    // [EH-02] all exceptions silently discarded; caller gets false
    {              //         with no diagnosis. At minimum log, or std::throw_with_nested.
        return false;
    }
}

// ---------------------------------------------------------------------------
// [EH-03] Throwing a raw string / int instead of a proper exception type.
// ---------------------------------------------------------------------------
inline void ParseConfig(const std::string& cfg)
{
    if (cfg.empty())
    {
        throw "config must not be empty";  // [EH-03] throwing const char*; cannot catch via
                                           //         std::exception; use std::invalid_argument
    }
    if (cfg.size() > 4096)
    {
        throw 42;  // [EH-03] throwing int; completely non-standard
    }
}

// ---------------------------------------------------------------------------
// [EH-04] Exception used for normal (non-error) control flow.
// ---------------------------------------------------------------------------
inline int FindIndex(const std::vector<int>& v, int target)
{
    // [EH-04] Throws when target is simply not found – a common, non-exceptional case.
    //         Should return std::optional<std::size_t> or -1 / a sentinel.
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        if (v[i] == target) return static_cast<int>(i);
    }
    throw std::runtime_error("element not found");   // not an error; normal outcome
}

// ---------------------------------------------------------------------------
// [EH-05] noexcept function calling a potentially throwing function.
// ---------------------------------------------------------------------------
inline std::string ReadLine(const std::string& path) noexcept  // [EH-05] promises noexcept
{
    std::ifstream f(path);
    std::string   line;
    std::getline(f, line);   // can throw; if it does, std::terminate is called
    return line;
}

// ---------------------------------------------------------------------------
// [EH-06] Re-throwing by value slices the exception.
// ---------------------------------------------------------------------------
inline void Process()
{
    try
    {
        throw std::runtime_error("base error");
    }
    catch (const std::exception& ex)
    {
        // [EH-06] Copies and slices the exception; dynamic type lost.
        //         Use bare 'throw;' to re-throw the original object in-place.
        std::runtime_error copy = std::runtime_error(ex.what());
        throw copy;
    }
}

// ---------------------------------------------------------------------------
// [EH-07] Error code returned as plain int – easily ignored by caller.
// ---------------------------------------------------------------------------
// [EH-07] Returning int error codes is fragile; use [[nodiscard]] + error enum,
//         std::error_code, or expected<T,E>.  No nodiscard attribute here.
int InitDevice(int device_id);

inline int InitDevice(int /*device_id*/)
{
    return -1;   // error; caller may write: InitDevice(3);  with no warning
}

// ---------------------------------------------------------------------------
// [EH-08] Constructor silently swallowing failure – half-initialised object.
// ---------------------------------------------------------------------------
class SilentlyFailingResource
{
public:
    explicit SilentlyFailingResource(const std::string& path)
    {
        file_.open(path);
        if (!file_.is_open())
        {
            // [EH-08] Should throw std::runtime_error or propagate via factory function.
            //         Instead, object is constructed in an invalid state;
            //         callers have no way to know without checking IsValid().
        }
        // No throw → object silently broken
    }

    bool IsValid() const { return file_.is_open(); }  // callers must remember to check

    std::string ReadLine()
    {
        if (!IsValid()) return {};  // silent failure
        std::string line;
        std::getline(file_, line);
        return line;
    }

private:
    std::ifstream file_;
};

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_EXCEPTION_ERROR_HANDLING_H
