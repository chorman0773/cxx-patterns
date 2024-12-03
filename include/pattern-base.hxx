#pragma once

#include <type_traits>
#include <concepts>
#include <tuple.hxx>
#include <cstddef>
#include <optional>

namespace cxx_patterns
{
    /// @brief  A Sentinel type that can be used by pattern types to match nothing
    /// It has a specialization of `tuple_size` for `0`
    using empty_matcher_t = std::tuple<>;

    constexpr inline empty_matcher_t empty_matcher{};

    namespace _detail
    {
        template <typename Opt>
        struct _optional_of_tuple_impl : std::false_type
        {
        };
        template <typename T>
        struct _optional_of_tuple_impl<std::optional<T>> : std::true_type
        {
        };

        template <typename Opt>
        concept _optional_of_tuple = _optional_of_tuple_impl<std::remove_cvref_t<Opt>>::value;

        template <typename Pat, typename T>
        concept _has_match_member = requires(const Pat &_pat, T &&_val) {
            _pat.match(std::forward<T>(_val));
        };

        template <typename Pat, typename T>
            requires(!(std::integral<Pat> && std::integral<std::remove_cvref_t<T>>) && !_has_match_member<Pat, T>)
        void match(const Pat &_pat, T &&_val) = delete;

        template <std::integral Pat, std::integral I>
        constexpr std::optional<empty_matcher_t> match(const Pat &pat, const I &val) noexcept
        {
            if (pat == val)
                return empty_matcher;
            else
                return std::nullopt;
        }

        template <typename Pat, typename T>
            requires _has_match_member<Pat, T>
        constexpr auto match(const Pat &_pat, T &&_val) noexcept(noexcept(_pat.match(std::forward<T>(_val)))) -> decltype(_pat.match(std::forward<T>(_val)))
        {
            return _pat.match(std::forward<T>(_val));
        }

        constexpr inline auto _match = [](const auto &_pat, auto &&_val) constexpr noexcept(noexcept(match(_pat, std::forward<decltype(_val)>(_val)))) -> decltype(match(_pat, std::forward<decltype(_val)>(_val)))
        {
            return match(_pat, std::forward<decltype(_val)>(_val));
        };
    }

    /// @brief A function-like object which, when called, tries to match `val` against `pat
    ///
    /// Call Signature: `template<typename Pat, typename T> /*see below*/ match(const Pat& pat, T&& val)`
    ///
    /// This is a function-like object. It satisfies the following properties:
    /// 1. Explicit template parameters cannot be provided.
    /// 2. It cannot be found by argument dependant lookup.
    /// 3. If found by unqualified lookup in the namespace `cxx_patterns`, it inhibits argument dependent lookup.
    ///
    /// If `std::remove_cvref_t<T>` satisfies `std::integral`, and `Pat` implicitly converts in a constexpr context to `std::remove_cvref_t<T>`,
    ///  then returns an optional containing `empty_matcher{}` if `pat == val`, otherwise returns an empty optional.
    ///
    /// Otherwise, A call to `match` is equivalent to first of the following that is well-formed. If none are well-formed, then the call to `match` is ill-formed:
    /// 1. The call `match(pat, std::forward<T>(val))` evaluated in a context that does not include this declaration, and includes `template<typename Pat, typename T> void match(const Pat& _pat, T&& _val) = delete`,
    /// 2. The call `pat.match(std::forward<T>(val))` evaluated in a context that is not a friend of `Pat` or `T`.
    constexpr inline auto match_pattern = _detail::_match;

    /// @brief A concept for types that can match on `T`
    ///
    /// A type `Pat` satisfies `pattern<T>` for a given type `T` if, given a const-qualified object of type `Pat` `pat` and a value of type `T` `t`, the call `match_pattern(pat, t)` is well-formed
    /// and returns `std::optional<Tuple>` where `Tuple` is some type that satisfies `tuple_access`
    ///
    /// A type `Pat` models `pattern<T>` for a given type `T` if `Pat` satisfies `pattern<T>`, the call mentioned above is *equality preserving* and does not modify `pat`, and type `Tuple` mentioend above
    /// models `tuple_access`. Additionally, `_val` must not be modified by the call to `match_pattern`, and the lifetime of every reference in `Tuple` is at least as long as the lifetime of `_val`.
    template <typename Pat, typename T>
    concept pattern = requires(const Pat &_pat, T &&_val) {
        { cxx_patterns::match_pattern(_pat, std::forward<T>(_val)) } -> _detail::_optional_of_tuple;
    };

    template <typename Pat, typename T>
    concept noexcept_pattern = pattern<Pat, T> && requires(const Pat &_pat, T &&_val) {
        { cxx_patterns::match_pattern(_pat, std::forward<T>(_val)) } noexcept;
    };

    template <typename Pat = void>
    struct binding
    {
        Pat _m_pat;

        template <typename T>
        auto match(T &&_val) const noexcept(noexcept(cxx_patterns::noexcept_pattern<Pat, T>))
            requires cxx_patterns::pattern<Pat, T>
        {
            if (auto _matched = cxx_patterns::match_pattern(this->_m_pat, std::forward<T>(_val)))
                return std::optional{std::tuple_cat(std::forward_as_tuple(std::forward<T>()), cxx_patterns::forward_to_tuple(std::move(*_matched)))};
            else
                return std::nullopt;
        }
    };

    template <>
    struct binding<void>
    {
        template <typename T>
        std::optional<std::tuple<T &&>> match(T &&_val) const noexcept
        {
            return std::forward_as_tuple(std::forward<T>(_val));
        }
    };

    template <typename Pat>
    explicit binding(Pat) -> binding<Pat>;

    explicit binding() -> binding<void>;

    namespace _detail
    {
        template <typename R>
        struct _unwrap_optional
        {
        };

        template <typename R>
        struct _unwrap_optional<std::optional<R>>
        {
            using type = R;
        };
    }

    template <typename Pat, typename T>
        requires pattern<Pat, T>
    struct pattern_outputs : _detail::_unwrap_optional<decltype(match_pattern(std::declval<const Pat &>(), std::declval<T>()))>
    {
    };

    template <typename Pat, typename T>
        requires pattern<Pat, T>
    using pattern_outputs_t = typename pattern_outputs<Pat, T>::type;
}

namespace std
{
    template <>
    struct tuple_size<cxx_patterns::empty_matcher_t> : std::integral_constant<std::size_t, 0>
    {
    };
}