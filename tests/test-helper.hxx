#pragma once

#include <span>
#include <string_view>
#include <ranges>
#include <expected>
#include <any>
#include <type_traits>
#include <typeinfo>
#include <string>
#include <string_view>
#include <variant>
#include <functional>
#include <cstddef>
#include <concepts.hxx>
#include <format>
#include <source_location>
#include <stdexcept>
#include <stacktrace>
#include <memory>
#include <iostream>
#include <mutex>
#include <cstdlib>

namespace cxx_tests
{

    namespace _detail
    {

        bool &_backtrace_flag(bool init) noexcept
        {
            using namespace std::string_view_literals;
            constinit static bool _s_backtrace_enabled{};
            constinit static std::once_flag _s_backtrace_init{};

            std::call_once(_s_backtrace_init, [init]() noexcept
                           {

                            if(!init)
                                return;
                if(char* _var = std::getenv("TEST_BACKTRACE")){
                    std::string_view _bt{_var};

                    if(_bt == "1"sv || _bt == "yes"sv || _bt == "y"sv)
                        _s_backtrace_enabled = true;
                } });

            return _s_backtrace_enabled;
        }

        template <typename T>
        struct _test_impl;
    }

    bool backtrace_enabled() noexcept
    {
        return _detail::_backtrace_flag(true);
    }

    struct captured_backtrace : std::exception
    {
    private:
        std::shared_ptr<std::stacktrace> _m_trace;

    protected:
        captured_backtrace(std::size_t _skip_head = 1) : _m_trace{std::make_shared<std::stacktrace>(backtrace_enabled() ? std::stacktrace::current(_skip_head + 1) : std::stacktrace{})} {}

    public:
        const std::stacktrace &backtrace() const noexcept
        {
            return *_m_trace;
        }
    };

    void set_backtrace(bool _backtrace) noexcept
    {
        _detail::_backtrace_flag(false) = _backtrace;
    }

    template <typename F>
    struct test : _detail::_test_impl<test<F>>
    {
        F *_m_func;
        std::string_view _m_test_name;

        test(F *_func, std::string_view _test_name) : _m_func(_func), _m_test_name(_test_name) {}
    };

    namespace _detail
    {
        template <>
        struct _test_impl<test<void()>>
        {
            bool operator()(this const test<void()> &self) noexcept
            {
                using namespace std::string_view_literals;

                std::println(std::cerr, "Running Test {}"sv, self._m_test_name);
                try
                {
                    std::invoke(self._m_func);

                    std::println(std::cerr, "Test {}: Success"sv, self._m_test_name);
                    return true;
                }
                catch (captured_backtrace &e)
                {
                    std::println(std::cerr, "Test {} failed: {}"sv, self._m_test_name, e.what());
                    if (backtrace_enabled())
                        std::println(std::cerr, "{}"sv, e.backtrace());

                    return false;
                }
                catch (std::exception &e)
                {
                    std::println(std::cerr, "Test {} failed: {}"sv, self._m_test_name, e.what());

                    return false;
                }
            }
        };
    }

    template <typename ArgsView, typename... Tests>
    void _test_driver_impl(ArgsView &&args, test<Tests> &&...tests)
    {
        if (!(std::invoke(std::forward<test<Tests>>(tests)) && ...))
        {
            std::exit(1);
        }
    }

    using namespace std::string_view_literals;

    template <typename... Args>
    void test_assert(bool, std::format_string<Args...>, Args &&...);

    struct assertion_failed : captured_backtrace
    {
    private:
        std::shared_ptr<std::string> _m_what;

        assertion_failed(std::string_view _fmt_result, std::source_location _loc) : _m_what(std::make_shared<std::string>(std::format("[{}: {} ({}:{})]: Assertion failed: {}"sv, _loc.file_name(), _loc.function_name(), _loc.line(), _loc.column(), _fmt_result)))
        {
        }

        template <typename... Args>
        friend void test_assert(bool, std::format_string<Args...>, Args &&...);

    public:
        const char *what() const noexcept override
        {
            return _m_what->data();
        }
    };

    template <typename... Args>
    void test_assert(bool v, std::format_string<Args...> fmt, Args &&...args)
    {
        if (!v)
            throw assertion_failed(std::format(fmt, std::forward<Args>(args)...), std::source_location::current());
    }

}

#define EVAL0(...) __VA_ARGS__
#define EVAL(...) EVAL0(__VA_ARGS__)

#define make_test(_test) \
    test { _test, std::string_view(#_test) }

#define TEST_DRIVER(...)                                                                                                                    \
    int main(int argc, char **argv)                                                                                                         \
    {                                                                                                                                       \
        std::span<char *> args{argv, static_cast<std::size_t>(argc)};                                                                       \
        cxx_tests::_test_driver_impl(args | std::views::transform([](auto &&a) { return std::string_view{a}; }) __VA_OPT__(, __VA_ARGS__)); \
    }

#define test_assert_expr(e) \
    test_assert(static_cast<bool>(e), std::string_view{"{}"}, std::string_view{#e})
