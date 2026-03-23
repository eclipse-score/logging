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

/// @file template_metaprogramming_issues.h
/// @brief Intentional template-metaprogramming issues for review-tool evaluation.
///
/// Issues planted:
///   [TM-01] Reinvented type trait (`IsSameCustom`) instead of `std::is_same_v`.
///   [TM-02] Recursive template factorial where `constexpr` function is simpler.
///   [TM-03] SFINAE return-type `enable_if` causes poor diagnostics/readability.
///   [TM-04] Unconstrained template accepts unsupported types.
///   [TM-05] Partial specialization changes semantic contract unexpectedly.
///   [TM-06] Recursive variadic template sum instead of C++17 fold expression.
///   [TM-07] Hand-rolled typelist length recursion instead of tuple/pack facilities.
///   [TM-08] Pointer specialization ignores cv/ref categories.
///   [TM-09] Template bool parameter over-configures behavior (boolean trap at type level).
///   [TM-10] Missing `static_assert` for template preconditions.

#ifndef SCORE_EVALUATION_TEMPLATE_METAPROGRAMMING_ISSUES_H
#define SCORE_EVALUATION_TEMPLATE_METAPROGRAMMING_ISSUES_H

#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// [TM-01] Reinvented trait (unnecessary custom implementation)
// ---------------------------------------------------------------------------
template <typename A, typename B>
struct IsSameCustom
{
    static constexpr bool value = false;
};

template <typename A>
struct IsSameCustom<A, A>
{
    static constexpr bool value = true;
};

// ---------------------------------------------------------------------------
// [TM-02] Recursive factorial TMP for simple numeric constant
// ---------------------------------------------------------------------------
template <int N>
struct Factorial
{
    static constexpr int value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0>
{
    static constexpr int value = 1;
};

// ---------------------------------------------------------------------------
// [TM-03] SFINAE in return type (hard-to-read diagnostics)
// ---------------------------------------------------------------------------
template <typename T>
auto ToStringSfinae(const T& v) -> typename std::enable_if<std::is_integral<T>::value, std::string>::type
{
    return std::to_string(v);
}

template <typename T>
auto ToStringSfinae(const T& v) -> typename std::enable_if<std::is_floating_point<T>::value, std::string>::type
{
    return std::to_string(v);
}

// ---------------------------------------------------------------------------
// [TM-04] Unconstrained template (accepts too many types)
// ---------------------------------------------------------------------------
template <typename T>
T AddOneGeneric(T value)
{
    // Accepts any T with operator+, but intended only for arithmetic.
    return value + static_cast<T>(1);
}

// ---------------------------------------------------------------------------
// [TM-05] Specialization changes semantics unexpectedly
// ---------------------------------------------------------------------------
template <typename T>
struct ValuePolicy
{
    static T Default() { return T{}; }
};

template <>
struct ValuePolicy<bool>
{
    static bool Default() { return true; }  // semantic surprise vs generic zero-init behavior
};

// ---------------------------------------------------------------------------
// [TM-06] Recursive variadic template sum (pre-C++17 style)
// ---------------------------------------------------------------------------
template <typename T>
constexpr T SumRecursive(T value)
{
    return value;
}

template <typename T, typename... Rest>
constexpr T SumRecursive(T first, Rest... rest)
{
    return first + SumRecursive<T>(static_cast<T>(rest)...);
}

// ---------------------------------------------------------------------------
// [TM-07] Hand-rolled typelist length recursion
// ---------------------------------------------------------------------------
template <typename... Ts>
struct TypeListLength;

template <>
struct TypeListLength<>
{
    static constexpr std::size_t value = 0U;
};

template <typename T, typename... Ts>
struct TypeListLength<T, Ts...>
{
    static constexpr std::size_t value = 1U + TypeListLength<Ts...>::value;
};

// ---------------------------------------------------------------------------
// [TM-08] Pointer-only specialization ignores cv/ref qualifiers of pointee
// ---------------------------------------------------------------------------
template <typename T>
struct IsPointerLike
{
    static constexpr bool value = false;
};

template <typename T>
struct IsPointerLike<T*>
{
    static constexpr bool value = true;
};

// ---------------------------------------------------------------------------
// [TM-09] Boolean template parameter controls semantics (type-level bool trap)
// ---------------------------------------------------------------------------
template <typename T, bool ClampNegative>
struct Normalizer
{
    static T Apply(T value)
    {
        if (ClampNegative && value < static_cast<T>(0))
        {
            return static_cast<T>(0);
        }
        return value;
    }
};

// ---------------------------------------------------------------------------
// [TM-10] Missing static_assert preconditions in template API
// ---------------------------------------------------------------------------
template <typename T>
struct FixedBlock
{
    // Intended for POD-like types only, but no static_assert enforces this.
    T data[4];
};

// Utility calls to ensure templates are instantiated in translation units.
inline void EvaluateTemplateMetaprogrammingSamples()
{
    constexpr bool same = IsSameCustom<int, int>::value;
    (void)same;

    constexpr int fact5 = Factorial<5>::value;
    (void)fact5;

    const auto i = ToStringSfinae(42);
    const auto d = ToStringSfinae(3.14);
    (void)i;
    (void)d;

    const auto add = AddOneGeneric(7);
    (void)add;

    const auto b = ValuePolicy<bool>::Default();
    (void)b;

    constexpr int s = SumRecursive<int>(1, 2, 3, 4);
    (void)s;

    constexpr std::size_t len = TypeListLength<int, float, double>::value;
    (void)len;

    constexpr bool p = IsPointerLike<const int*>::value;
    (void)p;

    const auto n = Normalizer<int, true>::Apply(-7);
    (void)n;

    FixedBlock<std::tuple<int, int>> block{};
    (void)block;
}

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_TEMPLATE_METAPROGRAMMING_ISSUES_H
