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

/// @file code_optimization.h
/// @brief Missed optimisation opportunities for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §6 for expected review comments):
///   [CO-01] Copying a large container argument that should be passed by const-ref.
///   [CO-02] Repeated .size() call in loop condition that prevents loop-invariant hoisting.
///   [CO-03] Constructing temporary std::string from literal in hot path.
///   [CO-04] Redundant search: find() followed by operator[] (double lookup in map).
///   [CO-05] Returning large object by value from a function without move (pre-NRVO pattern).
///   [CO-06] Using std::list where std::vector would be far more cache-friendly.
///   [CO-07] Virtual dispatch inside tight inner loop – should be devirtualised.
///   [CO-08] Missing reserve() before push_back loop – causes repeated reallocations.
///   [CO-09] Sorting a container multiple times instead of once (redundant sort).
///   [CO-10] std::string concatenation in loop – quadratic complexity.

#ifndef SCORE_EVALUATION_CODE_OPTIMIZATION_H
#define SCORE_EVALUATION_CODE_OPTIMIZATION_H

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// [CO-01] Large container passed by value – unnecessary deep copy.
// ---------------------------------------------------------------------------
inline int SumLargeVector(std::vector<int> data)   // [CO-01]: should be const std::vector<int>&
{
    int sum = 0;
    for (auto v : data) sum += v;
    return sum;
}

// ---------------------------------------------------------------------------
// [CO-02] .size() called on every iteration – may prevent optimisation.
// ---------------------------------------------------------------------------
inline void ProcessItems(const std::vector<std::string>& items)
{
    for (std::size_t i = 0; i < items.size(); ++i)   // [CO-02]: cache size before loop
    {
        (void)items[i];  // simulate work
    }
}

// ---------------------------------------------------------------------------
// [CO-03] Temporary std::string constructed from literal in hot-path comparison.
// ---------------------------------------------------------------------------
inline bool IsError(const std::string& level)
{
    return level == std::string("ERROR");  // [CO-03]: literal comparison creates a temporary;
                                           //          use level == "ERROR" directly or string_view
}

// ---------------------------------------------------------------------------
// [CO-04] Double lookup in std::map – find + operator[].
// ---------------------------------------------------------------------------
inline void IncrementCounter(std::map<std::string, int>& counters, const std::string& key)
{
    if (counters.find(key) != counters.end())   // first lookup
    {
        counters[key]++;                         // [CO-04]: second lookup; use iterator from find
    }
    else
    {
        counters[key] = 1;                       // third lookup on insertion path
    }
}

// ---------------------------------------------------------------------------
// [CO-05] Large object built then copied out without relying on NRVO.
// ---------------------------------------------------------------------------
inline std::vector<int> BuildRange(int n)
{
    std::vector<int> result;
    // [CO-08] also: no reserve() before push_back loop
    for (int i = 0; i < n; ++i) result.push_back(i);

    std::vector<int> copy = result;   // [CO-05]: unnecessary copy; return result directly
    return copy;
}

// ---------------------------------------------------------------------------
// [CO-06] std::list used where std::vector is more appropriate.
// ---------------------------------------------------------------------------
inline std::list<int> CollectSamples(int n)
{
    std::list<int> samples;   // [CO-06]: std::list has poor cache locality;
                               //          use std::vector unless frequent mid-insertion needed
    for (int i = 0; i < n; ++i) samples.push_back(i);
    return samples;
}

// ---------------------------------------------------------------------------
// [CO-07] Virtual dispatch in tight inner loop.
// ---------------------------------------------------------------------------
class ITransform
{
public:
    virtual ~ITransform() = default;
    virtual int Apply(int x) const = 0;
};

class DoubleTransform : public ITransform
{
public:
    int Apply(int x) const override { return x * 2; }
};

inline void ApplyTransforms(std::vector<int>& data, const ITransform* transform)
{
    // [CO-07] Virtual call per element in a potentially large vector.
    //         Consider passing a concrete type via template parameter.
    for (auto& v : data) v = transform->Apply(v);
}

// ---------------------------------------------------------------------------
// [CO-09] Redundant sort – container sorted multiple times.
// ---------------------------------------------------------------------------
inline std::vector<int> SortAndSlice(std::vector<int> data)
{
    std::sort(data.begin(), data.end());   // first sort

    // ... some processing that doesn't break order ...

    std::sort(data.begin(), data.end());   // [CO-09]: redundant second sort

    if (data.size() > 10) data.resize(10);
    return data;
}

// ---------------------------------------------------------------------------
// [CO-10] String concatenation in loop – O(n^2) behaviour.
// ---------------------------------------------------------------------------
inline std::string JoinLines(const std::vector<std::string>& lines)
{
    std::string result;
    for (const auto& line : lines)
    {
        result = result + line + "\n";  // [CO-10]: creates a new string object each iteration;
                                         //          use result += line; result += '\n'; or std::ostringstream
    }
    return result;
}

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_CODE_OPTIMIZATION_H
