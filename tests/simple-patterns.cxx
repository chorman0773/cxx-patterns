
#include "test-helper.hxx"

#include <pattern-base.hxx>

void test_match_int()
{
    cxx_tests::test_assert_expr(cxx_patterns::match_pattern(5, 5));
}

void test_match_int_mismatch()
{
    cxx_tests::test_assert_expr(!cxx_patterns::match_pattern(5, 6));
}

void test_match_any()
{
    constexpr static int matched{5};
    auto bind = cxx_patterns::match_pattern(cxx_patterns::binding{}, matched);

    cxx_tests::test_assert_expr(bind);

    auto &&[x] = *bind;

    cxx_tests::test_assert_expr(x == 5);
}

TEST_DRIVER(cxx_tests::make_test(test_match_int), cxx_tests::make_test(test_match_int_mismatch), cxx_tests::make_test(test_match_any))
