#include "test-helper.hxx"

#include <empty.hxx>
#include <type-traits.hxx>

static_assert(std::is_same_v<int, std::common_type_t<cxx_patterns::empty, int>>);
static_assert(std::is_same_v<int, std::common_type_t<int, cxx_patterns::empty>>);

TEST_DRIVER();