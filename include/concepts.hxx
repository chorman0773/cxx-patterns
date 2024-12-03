#pragma once

#include <concepts>

namespace cxx_patterns
{
    template <typename Bool>
    concept _bool_like_impl = requires(const Bool &b) {
        { static_cast<bool>(b) } noexcept;
        !b;
    };

    /// @brief a concept for types that act like `bool`
    ///
    /// A type `B` satisfies `bool_like` if it can be explicitly converted to `bool`, and, given two objects of type (potentially const-qualified) `Bool` b and c, and a value of type `bool` d,
    ///  the following expressions are well formed, returning a type that can be explicitly converted to `bool`:
    /// * `!b`
    /// * `b || c`
    /// * `b && c`
    /// And the following expressions are well formed and return `bool`:
    /// * `b || d`
    /// * `d || c`
    /// * `b && d`
    /// * `d || c`
    ///
    /// A type `B` models `bool_like` if `B` satisfies `bool_like`, every expression mentioned above does not throw any exceptions, is *equality preserving*, and returns a type which *models* `bool_like,
    /// the expressions in the second list all invoke built in operators, and each expression above returns the same result and has no side effects (observable behaviour or scalar writes)
    /// different from if any operand `e` of type `B` in the expression was replaced with `static_cast<bool>(e)`.
    template <typename B>
    concept bool_like = _bool_like_impl<B> && requires(const B &b, const B &c, bool d) {
        { !b } -> _bool_like_impl;
        { b || c } -> _bool_like_impl;
        { b &&c } -> _bool_like_impl;
        { b || d } -> std::same_as<bool>;
        { d || b } -> std::same_as<bool>;
        { b &&d } -> std::same_as<bool>;
        { d &&b } -> std::same_as<bool>;
    };
}