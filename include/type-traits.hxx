#pragma once
#include <type_traits>

namespace cxx_patterns
{
    template <typename T, typename U>
    struct forward_cvref : std::type_identity<T>
    {
    };

    template <typename T, typename U>
    struct forward_cvref<T, const U> : std::type_identity<const T>
    {
    };

    template <typename T, typename U>
    struct forward_cvref<T, volatile U> : std::type_identity<volatile T>
    {
    };

    template <typename T, typename U>
    struct forward_cvref<T, const volatile U> : std::type_identity<const volatile T>
    {
    };

    template <typename T, typename U>
    struct forward_cvref<T, U &> : std::type_identity<typename forward_cvref<T, U>::type &>
    {
    };

    template <typename T, typename U>
    struct forward_cvref<T, U &&> : std::type_identity<typename forward_cvref<T, U>::type &&>
    {
    };

    template <typename T, typename U>
    using forward_cvref_t = typename forward_cvref<T, U>::type;
}