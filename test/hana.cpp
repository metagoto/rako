#ifdef __clang__
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wcomma"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif

#include <boost/hana.hpp>
#include "./simple_test.hpp"

#include <numeric>
#include <string_view>
#include <utility>

using namespace std;
namespace h = boost::hana;

template <typename... T>
constexpr auto has_dup() {
  constexpr auto t = h::tuple_t<T...>;
  return h::size(t) != h::size(h::to_set(t));
}

auto test1() {
  using h::tuple_t;

  static_assert(has_dup<int, float, int>());
  static_assert(!has_dup<int, float, char>());
}

int main() {
  test1();
  return ::test_result();
}
