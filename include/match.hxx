#pragma once

#include <pattern-base.hxx>
#include <concepts>
#include <type_traits>
#include <tuple.hxx>
#include <utility>
#include <optional>
#include <string_view>
#include <empty.hxx>

namespace cxx_patterns
{
    template <typename Pat>
    struct matcher
    {
    private:
        Pat _m_pat;

    public:
        template <typename TPat>
            requires(std::same_as<std::remove_cvref_t<Pat>, std::remove_cvref_t<TPat>> && std::convertible_to<TPat &&, Pat>)
        consteval matcher(TPat &&pat) : _m_pat{std::forward<TPat>(pat)}
        {
        }

        template <typename T>
            requires cxx_patterns::pattern<Pat, T>
        constexpr auto match(T &&_val) const noexcept(cxx_patterns::noexcept_pattern<Pat, T>)
        {
            return cxx_patterns::match_pattern(this->_m_pat, _val);
        }
    };

    template <typename CharT, typename CharTraits>
    struct matcher<std::basic_string_view<CharT, CharTraits>>
    {
    private:
        std::basic_string_view<CharT, CharTraits> _m_pat;

    public:
        consteval matcher(std::basic_string_view<CharT, CharTraits> pat) : _m_pat{pat}
        {
        }

        constexpr std::optional<empty_matcher_t> match(std::basic_string_view<CharT, CharTraits> _val) noexcept
        {
            if (this->_m_pat == _val)
                return empty_matcher;
            else
                return std::nullopt;
        }
    };

    template <typename... Pats>
    struct matcher<std::tuple<Pats...>>
    {
    private:
        std::tuple<Pats...> _m_pat;

    public:
        template <typename Tuple>
            requires(std::same_as<std::remove_cvref_t<std::tuple<Pats...>>, std::remove_cvref_t<Tuple>> && std::convertible_to<Tuple &&, std::tuple<Pats...>>)
        consteval matcher(Tuple &&pat) : _m_pat{std::forward<Tuple>(pat)}
        {
        }

        template <typename... Ts, std::size_t... Is>
            requires(cxx_patterns::pattern<Pats, Ts> && ...)
        constexpr auto _match_impl(std::tuple<Ts &&...> _tup, std::index_sequence<Is...>) const noexcept((cxx_patterns::noexcept_pattern<Pats, Ts> && ...))
        {
            auto _binders{std::make_tuple(cxx_patterns::match_pattern(std::get<Is>(this->_m_pat), std::forward<Ts>(std::get<Is>(_tup)))...)};

            if ((std::get<Is>(_binders) && ...))
                return std::optional{std::tuple_cat(std::move(*std::get<Is>(_binders))...)};
            else
                return std::nullopt;
        }

        template <cxx_patterns::tuple_like Tuple>
        constexpr auto match(Tuple &&_tup) const noexcept(noexcept(this->_match_impl(cxx_patterns::forward_to_tuple(std::forward<Tuple>(_tup)), std::make_index_sequence<std::tuple_size_v<Tuple>>{})))
            -> decltype(this->_match_impl(cxx_patterns::forward_to_tuple(std::forward<Tuple>(_tup)), std::make_index_sequence<std::tuple_size_v<Tuple>>{}))
        {
            return this->_match_impl(cxx_patterns::forward_to_tuple(std::forward<Tuple>(_tup)), std::make_index_sequence<std::tuple_size_v<Tuple>>{});
        }
    };

    template <typename Pat, typename T, typename F>
    concept matchable = cxx_patterns::pattern<Pat, T> && cxx_patterns::applyable<F &&, cxx_patterns::pattern_outputs_t<Pat, T>>;

    namespace _detail
    {
        struct _regular_void
        {
        };

        template <typename Pat, typename T, typename F>
        concept _matchable_void = cxx_patterns::matchable<Pat, T, F> &&
                                  std::same_as<void,
                                               cxx_patterns::apply_result_t<F &&, cxx_patterns::pattern_outputs_t<Pat, T>>>;
    }

    template <typename Pat, typename F>
    struct match_arm
    {
        cxx_patterns::matcher<Pat> _m_pat;
        F _m_arm;

        template <typename Self, typename T>
            requires cxx_patterns::matchable<Pat, T, cxx_patterns::forward_cvref_t<F, Self>> && (!_detail::_matchable_void<Pat, T, cxx_patterns::forward_cvref_t<F, Self>>)
        constexpr auto
        operator()(this Self &&self, T &&t) noexcept((cxx_patterns::noexcept_pattern<Pat, T> && cxx_patterns::noexcept_applyable<cxx_patterns::forward_cvref_t<F, Self>, cxx_patterns::pattern_outputs_t<Pat, T>>))
        {
            return cxx_patterns::match_pattern(self._m_pat, std::forward<T>(t))
                .transform([&self](auto &&_tuple)
                           { return std::apply(std::forward<cxx_patterns::forward_cvref_t<F, Self>>(self._m_arm), std::move(_tuple)); });
        }

        template <typename Self, typename T>
            requires cxx_patterns::matchable<Pat, T, cxx_patterns::forward_cvref_t<F, Self>> && _detail::_matchable_void<Pat, T, cxx_patterns::forward_cvref_t<F, Self>>
        constexpr std::optional<_detail::_regular_void>
        operator()(this Self &&self, T &&t) noexcept((cxx_patterns::noexcept_pattern<Pat, T> && cxx_patterns::noexcept_applyable<cxx_patterns::forward_cvref_t<F, Self>, cxx_patterns::pattern_outputs_t<Pat, T>>))
        {
            return cxx_patterns::match_pattern(self._m_pat, std::forward<T>(t))
                .transform([&self](auto &&_tuple)
                           { std::apply(std::forward<cxx_patterns::forward_cvref_t<F, Self>>(self._m_arm), std::move(_tuple)); return _detail::_regular_void{}; });
        }
    };

    template <typename Pat, typename F>
    match_arm(Pat, F) -> match_arm<Pat, F>;

    namespace _detail
    {

        template <typename R, typename T>
        R _match_fn_impl(T &&_scrutinee) noexcept(true)
        {
            std::unreachable();
        }

        template <typename T, typename... M>
        using _match_impl_return_type = std::common_type_t<empty, typename _unwrap_optional<std::invoke_result_t<M, T &&>>::type...>;

        template <typename R, typename T, typename Pat1, typename F1, typename... Pat, typename... F>
        R
        _match_fn_impl(T &&_scrutinee, match_arm<Pat1, F1> &&_arm1, match_arm<Pat, F> &&..._arms) noexcept(std::is_nothrow_invocable_v<match_arm<Pat1, F1>, T &&> && (std::is_nothrow_invocable_v<match_arm<Pat, F>, T &&> && ...))
        {
            if (auto _arm_match = _arm1(std::move(_scrutinee)))
                return static_cast<R>(*_arm_match);
            else
                return _match_fn_impl<R>(std::move(_scrutinee), std::move(_arms)...);
        }

    }

    template <typename T, typename... Pat, typename... F>
        requires(cxx_patterns::matchable<Pat, T, F &&> && ... && true) && (!std::same_as<_detail::_match_impl_return_type<T, match_arm<Pat, F>...>, _detail::_regular_void>)
    auto match(T &&_scrutinee, match_arm<Pat, F> &&..._arms) noexcept((std::is_nothrow_invocable_v<match_arm<Pat, F>, T &&> && ... && true))
    {
        return _detail::_match_fn_impl<_detail::_match_impl_return_type<T, match_arm<Pat, F>...>>(std::forward<T>(_scrutinee), std::move(_arms)...);
    }

    template <typename T, typename... Pat, typename... F>
        requires(cxx_patterns::matchable<Pat, T, F &&> && ... && true) && (std::same_as<_detail::_match_impl_return_type<T, match_arm<Pat, F>...>, _detail::_regular_void>)
    void match(T &&_scrutinee, match_arm<Pat, F> &&..._arms) noexcept((std::is_nothrow_invocable_v<match_arm<Pat, F>, T &&> && ... && true))
    {
        _detail::_match_fn_impl<_detail::_regular_void>(std::forward<T>(_scrutinee), std::move(_arms)...);
    }
}
