#include <match.hxx>

#include "test-helper.hxx"

using namespace std::string_view_literals;

void test_match_exact()
{

    cxx_patterns::match(5,
                        cxx_patterns::match_arm{5, []() {}},
                        cxx_patterns::match_arm{cxx_patterns::binding{}, [](auto _val)
                                                {
                                                    cxx_tests::test_assert(false, "match 5"sv);
                                                }});
}

void test_match_fallthrough()
{
    cxx_patterns::match(4,
                        cxx_patterns::match_arm{5, []()
                                                {
                                                    cxx_tests::test_assert(false, "match 4"sv);
                                                }},
                        cxx_patterns::match_arm{cxx_patterns::binding{}, [](auto _val) {

                                                }});
}

void test_match_does_not()
{
    cxx_patterns::match(4,
                        cxx_patterns::match_arm{5, []()
                                                {
                                                    cxx_tests::test_assert(false, "match 4"sv);
                                                }},
                        cxx_patterns::match_arm(4, []() {

                        }),
                        cxx_patterns::match_arm{cxx_patterns::binding{}, [](auto _val)
                                                {
                                                    cxx_tests::test_assert(false, "match 4"sv);
                                                }});
}

TEST_DRIVER(cxx_tests::make_test(test_match_exact), cxx_tests::make_test(test_match_fallthrough), cxx_tests::make_test(test_match_does_not));