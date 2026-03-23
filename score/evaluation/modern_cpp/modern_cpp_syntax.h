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

/// @file modern_cpp_syntax.h
/// @brief Pre-C++11/14 patterns that should be replaced with C++11/14/17 idioms.
///
/// Issues planted (see EVALUATION_GUIDE.md §4 for expected review comments):
///   [MC-01] Raw loop with iterator instead of range-for.
///   [MC-02] std::auto_ptr instead of std::unique_ptr (removed in C++17).
///   [MC-03] Explicit functor/struct for predicate instead of lambda.
///   [MC-04] Manual type-tag enum instead of std::variant (C++17).
///   [MC-05] Missing noexcept on move constructor/assignment.
///   [MC-06] Typedef instead of using alias.
///   [MC-07] Manual pair return instead of structured bindings (C++17).
///   [MC-08] Unchecked std::map::operator[] (inserts default) instead of find/at.
///   [MC-09] std::bind instead of lambda capture.
///   [MC-10] Missing [[nodiscard]] on error-signalling return value.

#ifndef SCORE_EVALUATION_MODERN_CPP_SYNTAX_H
#define SCORE_EVALUATION_MODERN_CPP_SYNTAX_H

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// [MC-06] typedef instead of using alias.
// ---------------------------------------------------------------------------
typedef std::vector<std::string> StringList;           // [MC-06]: use using StringList = ...
typedef std::map<std::string, int> CounterMap;         // [MC-06]

// ---------------------------------------------------------------------------
// [MC-01] Raw iterator loop instead of range-for.
// ---------------------------------------------------------------------------
inline int CountNonEmpty(const StringList& items)
{
    int count = 0;
    for (StringList::const_iterator it = items.begin(); it != items.end(); ++it)  // [MC-01]
    {
        if (!it->empty())
        {
            ++count;
        }
    }
    return count;
}

// ---------------------------------------------------------------------------
// [MC-03] Explicit functor struct instead of lambda.
// ---------------------------------------------------------------------------
struct IsLongString  // [MC-03]: replace with inline lambda
{
    bool operator()(const std::string& s) const { return s.size() > 10; }
};

inline StringList FilterLong(const StringList& items)
{
    StringList result;
    std::copy_if(items.begin(), items.end(), std::back_inserter(result), IsLongString{});
    return result;
}

// ---------------------------------------------------------------------------
// [MC-04] Manual type-tag enum instead of std::variant.
// ---------------------------------------------------------------------------
enum class ValueType { INT, DOUBLE, STRING };

struct TaggedValue         // [MC-04]: should use std::variant<int, double, std::string>
{
    ValueType  tag;
    int        int_val;
    double     dbl_val;
    std::string str_val;
};

inline void PrintTaggedValue(const TaggedValue& v)
{
    if (v.tag == ValueType::INT)         { /* use v.int_val */ }
    else if (v.tag == ValueType::DOUBLE) { /* use v.dbl_val */ }
    else                                  { /* use v.str_val */ }
}

// ---------------------------------------------------------------------------
// [MC-05] Move constructor/assignment missing noexcept.
// ---------------------------------------------------------------------------
class MovableResource
{
public:
    explicit MovableResource(std::size_t n) : size_(n), data_(new int[n]) {}

    MovableResource(MovableResource&& other)   // [MC-05]: missing noexcept – prevents
        : size_(other.size_), data_(other.data_)  //   std::vector from using move optimisation
    {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    MovableResource& operator=(MovableResource&& other)  // [MC-05]: missing noexcept
    {
        delete[] data_;
        data_       = other.data_;
        size_       = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    ~MovableResource() { delete[] data_; }

private:
    std::size_t size_;
    int*        data_;
};

// ---------------------------------------------------------------------------
// [MC-07] Returning pair instead of using structured bindings / dedicated struct.
// ---------------------------------------------------------------------------
inline std::pair<bool, int> FindFirst(const std::vector<int>& vec, int target)
{
    for (std::size_t i = 0; i < vec.size(); ++i)
    {
        if (vec[i] == target) return std::make_pair(true, static_cast<int>(i));
    }
    return std::make_pair(false, -1);
    // [MC-07]: C++17 caller should use auto [found, idx] = FindFirst(...);
    //          Better yet, return std::optional<std::size_t>
}

// ---------------------------------------------------------------------------
// [MC-08] std::map::operator[] inserts a default element silently.
// ---------------------------------------------------------------------------
inline int GetCount(CounterMap& counters, const std::string& key)
{
    return counters[key];  // [MC-08]: inserts 0 if key missing; use .find() or .at()
}

// ---------------------------------------------------------------------------
// [MC-09] std::bind instead of lambda.
// ---------------------------------------------------------------------------
inline std::function<int(int)> MakeAdder(int offset)
{
    // [MC-09]: prefer [offset](int x){ return x + offset; }
    return std::bind(std::plus<int>(), std::placeholders::_1, offset);
}

// ---------------------------------------------------------------------------
// [MC-10] Missing [[nodiscard]] on a function whose return value signals errors.
// ---------------------------------------------------------------------------
bool InitialiseSubsystem(int flags);   // [MC-10]: missing [[nodiscard]]

// Implementation (would be in .cpp):
inline bool InitialiseSubsystem(int /*flags*/)
{
    return false;  // caller may silently ignore the failure
}

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_MODERN_CPP_SYNTAX_H
