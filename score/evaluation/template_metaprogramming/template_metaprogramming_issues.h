

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

template <typename T>
T AddOneGeneric(T value)
{
    return value + static_cast<T>(1);
}

template <typename T>
struct ValuePolicy
{
    static T Default() { return T{}; }
};

template <>
struct ValuePolicy<bool>
{
    static bool Default() { return true; }
};

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

template <typename T>
struct FixedBlock
{
    T data[4];
};

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

}
}

#endif  // SCORE_EVALUATION_TEMPLATE_METAPROGRAMMING_ISSUES_H
