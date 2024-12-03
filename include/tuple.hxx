/// Synopsis: <tuple.hxx>
/// ```
/// #include <tuple>
/// namespace cxx_patterns {
///     constexpr inline auto get;;
///     template<typename Tuple> concept tuple_like;
///     template<tuple_like Tuple> constexpr std::tuple</*see below*/> forward_to_tuple(Tuple&&) noexcept;
/// }
///```

#pragma once

#include <tuple>

#include <utility>
#include <concepts>
#include <variant>

#include <type-traits.hxx>

namespace cxx_patterns
{
    namespace _detail
    {

        template <typename T>
        struct _is_not_variant_impl : std::true_type
        {
        };
        template <typename... Ts>
        struct _is_not_variant_impl<std::variant<Ts...>> : std::false_type
        {
        };

        template <typename T>
        concept _is_not_variant = _is_not_variant_impl<T>::value;

        template <std::size_t I, typename T>
        void get(const T &&) = delete;

        template <std::size_t I, typename T>
            requires requires(T &&t) {
                std::forward<T>(t).template get<I>();
            }
        constexpr decltype(auto) get(T &&t) noexcept(noexcept(std::forward<T>(t).template get<I>()))
        {
            return std::forward<T>(t).template get<I>();
        }

        template <typename std::size_t I>
        constexpr inline auto _get = []<_is_not_variant T>(T &&a) noexcept(noexcept(get<I>(std::forward<decltype(a)>(a)))) -> decltype(get<I>(std::forward<decltype(a)>(a)))
        {
            using std::get;
            return get<I>(std::forward<decltype(a)>(a));
        };

        template <typename Tuple, std::size_t... I>
        constexpr std::tuple<forward_cvref_t<std::tuple_element_t<I, Tuple>, Tuple &&>...>
        _to_tuple_impl(Tuple &&t, std::index_sequence<I...>) noexcept
        {
            return std::forward_as_tuple(_get<I>(std::forward<Tuple>(t))...);
        }
    }

    /// @brief  A function-like object that obtains values of type `T`
    ///
    /// Call Signature: `template<std::size_t I, typename Tuple> /*see below*/ get(Tuple&& t);`
    ///
    /// This is a function-like object. It satisfies the following properties:
    /// 1. Explicit template parameters cannot be provided, except for the initial template parameter of type `I`
    /// 2. It cannot be found by argument dependant lookup.
    /// 3. If found by unqualified lookup in the namespace `cxx_patterns`, it inhibits argument dependent lookup.
    ///
    /// If `Tuple` is a specialization of `std::variant`, then the program is ill-formed
    ///
    /// The call `cxx_patterns::get<I>(t)` where `t` is of type `Tuple` is equivalent to the first expression which is well-formed if `Tuple` is not a specialization of type `std::variant.
    /// If none of them are well-formed or `Tuple` is a specialization of type `std::variant`, the call to `get<I>` is ill-formed.
    /// 1. The call `get<I>(t)`, where the call is evaluated in a context that does not include this declaration, and includes `template<std::size_t I, typename T> void get(const T&)=delete;`.
    /// 2. The call `t.get<I>()` where the call is evaluated in a context that is not a friend of `Tuple`.
    template <std::size_t I>
    constexpr inline auto get = _detail::_get<I>;

    template <typename Tuple>
    concept _tuple_like = requires {
        { auto(std::tuple_size_v<std::remove_cvref_t<Tuple>>) } -> std::unsigned_integral;
    };

    /// @brief a Concept for types that provide a tuple-like interface.
    ///
    /// A type `T` satisfies `tuple_like` if (after removing top-level reference and cv-qualifiers) it is an array of known bound
    ///  or the class `std::tuple_size<std::remove_cvref_t<T>>` names a partial or explicit specialization of that class template.
    ///
    /// A type `T` models `tuple_like` if it satisfies `tuple_like` and it is either an array of known bound, or, given an expression of type `T` t,
    ///  for all `I` which is less than `std::tuple_size_v<std::remove_cvref_t<T>>` the following are well formed and equality preserving:
    /// * `std::tuple_element_t<I,std::remove_cvref_t<T>>`
    /// * `cxx_patterns::get<I>(t)`, which is an lvalue expression of `t` is, and is of type:
    ///    * `const std::tuple_element_t<I,std::remove_cvref_t<T>>` if `std::remove_reference_t<T>` is the same type as `const std::remove_cvref_t<T>`
    ///    * or `std::tuple_element_t<I,std::remove_cvref_t<T>>` if `std::remove_reference_t<T>` is the same type as `std::remove_cvref_t<T>`
    ///
    /// And no call `cxx_patterns::get<I>(t)` throws any exceptions.
    template <typename Tuple>
    concept tuple_like =
        _tuple_like<Tuple> || std::is_bounded_array_v<std::remove_cvref_t<Tuple>>;

    /// @brief Converts a `tuple_like` type to a `std::tuple` of references.
    /// If `std::remove_cvref_t<Tuple>` is an array of known bound `T[N]`, then returns `std::tuple<forward_cvref_t<T, Tuple>, forward_cvref_t<T, Tuple>, ..., forward_cvref_t<T, Tuple>>` (where the total number of type parameters for `std::tuple` is equal to `N`),
    /// Otherwise, like `Is...` designate a template parameter pack which is instantiated with `0, 1, ..., std::tuple_size_v<Tuple>`.
    /// The return type is `std::tuple<forward_cvref_t<std::tuple_element_t<Is, std::remove_cvref_t<Tuple>>...>`
    ///
    /// @pre `Tuple` shall model `tuple_like`. No diagnostic is required.
    /// @tparam Tuple a type that satisfies `tuple_like`
    /// @return  A tuple that contains references to each element of `tuple`
    template <_tuple_like Tuple>
    decltype(auto) forward_to_tuple(Tuple &&tuple) noexcept
    {
        return _detail::_to_tuple_impl(std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    }

    template <typename T, std::size_t N>
    decltype(auto) forward_to_tuple(T (&&arr)[N]) noexcept
    {
        return _detail::_to_tuple_impl(std::move(arr), std::make_index_sequence<N>{});
    }

    template <typename T, std::size_t N>
    decltype(auto) forward_to_tuple(T (&arr)[N]) noexcept
    {
        return _detail::_to_tuple_impl(arr, std::make_index_sequence<N>{});
    }

    template <typename F, typename Tuple>
    concept applyable = tuple_like<Tuple> && requires(F &&f, Tuple &&t) {
        std::apply(std::forward<F>(f), std::forward<Tuple>(t));
    };

    template <typename F, typename Tuple>
    concept noexcept_applyable = applyable<F, Tuple> && requires(F &&f, Tuple &&t) {
        { std::apply(std::forward<F>(f), std::forward<Tuple>(t)) } noexcept;
    };

    template <typename F, typename Tuple>
        requires applyable<F, Tuple>
    using apply_result_t = decltype(std::apply(std::declval<F>(), std::declval<Tuple>()));
}
