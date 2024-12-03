#pragma once
#include <type_traits>
#include <utility>

namespace cxx_patterns
{
    struct empty
    {
        empty() = delete;

        empty(const empty &) = default;

        empty(empty &&) = default;

        template <typename T>
        operator T() const
        {
            std::unreachable();
        }

        template <typename T>
        operator T &()
        {
            std::unreachable();
        }

        template <typename T>
            requires(!std::is_const_v<T>)
        operator T &() const = delete;

        template <typename T>
        operator const T &() const
        {
            std::unreachable();
        }

        template <typename T>
        operator T &&() &&
        {
            std::unreachable();
        }

        template <typename T>
        operator const T &&() const &&
        {
            std::unreachable();
        }

        template <typename T>
            requires(!std::is_const_v<T>)
        operator T &&() const = delete;
    };
}

namespace std
{
    template <typename T>
        requires std::same_as<T, std::decay_t<T>> && (!std::same_as<T, cxx_patterns::empty>)
    struct common_type<cxx_patterns::empty, T>
    {
        using type = T;
    };

    template <typename T>
        requires std::same_as<T, std::decay_t<T>> && (!std::same_as<T, cxx_patterns::empty>)
    struct common_type<T, cxx_patterns::empty>
    {
        using type = T;
    };
}