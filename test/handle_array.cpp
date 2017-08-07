
#include <rako/handle_array.hpp>
#include "./simple_test.hpp"

#include <string_view>
#include <utility>

using namespace std;
using namespace rako;

//
template <typename T, typename F>
auto print(T const& a, F f) {
  handle_array_debug dbg;
  dbg.lookup_traverse(a, [](auto v) {
    cout << " | ";
    if (v.next_free == T::handle::npos)
      cout << "x";
    else
      cout << v.next_free;
    cout << "," << v.data_index;
  });
  cout << endl;
  auto i = 0u;
  dbg.data_traverse(a, [&](auto v) {
    if (i++ == a.size())
      cout << " [  ";
    else
      cout << " |  ";
    cout << f(v) << " ";
  });
  cout << "\n" << endl;
}

template <typename T>
auto to_string(T const& a) {
  handle_array_debug dbg;
  string s;
  dbg.data_traverse(a, [&](auto v) { s += to_string(v); });
  return s;
}

//constexpr auto fv = [](auto v) { return v; };

template <typename... T>
auto ts(T&&... t) {
  return make_tuple(forward<T>(t)...);
}

template <typename A, typename... T, size_t... I, typename... U, typename... V, size_t... J>
auto erase_check_impl(A& a, tuple<T...> t, index_sequence<I...>, string_view s, tuple<U...> u,
                      tuple<V...> v, index_sequence<J...>) {
  a.clear();
  array<typename A::handle, sizeof...(T)> hs = {};
  (void(hs[I] = a.add(get<I>(t))), ...);
  CHECK(to_string(a) == s);
  if constexpr (sizeof...(J) > 0)
    (void((a.erase(hs[get<J>(u)]), CHECK(to_string(a) == get<J>(v)))), ...);
  //print(a, fv);
}

template <typename A, typename... T, typename... U, typename... V>
auto erase_check(A& a, tuple<T...> t, string_view s, tuple<U...> u, tuple<V...> v) {
  erase_check_impl(a, t, index_sequence_for<T...>{}, s, u, v, index_sequence_for<U...>{});
}

template <typename A, typename... U, typename... V, size_t... J>
auto add_check_impl(A& a, tuple<U...> u, tuple<V...> v, index_sequence<J...>) {
  (void((a.add(get<J>(u)), CHECK(to_string(a) == get<J>(v)))), ...);
  //print(a, fv);
}

template <typename A, typename... U, typename... V>
auto add_check(A& a, tuple<U...> u, tuple<V...> v) {
  add_check_impl(a, u, v, index_sequence_for<U...>{});
}

//

auto test1() {
  using array_t = handle_array<int, 5>;
  array_t a;

  erase_check(a, ts(0, 1, 2, 3, 4), "01234", ts(), ts());

  erase_check(a, ts(0, 1, 2, 3, 4), "01234", ts(0u), ts("41230"));
  add_check(a, ts(5), ts("41235"));

  erase_check(a, ts(0, 1, 2, 3, 4), "01234", ts(4u), ts("01234"));
  add_check(a, ts(5), ts("01235"));

  erase_check(a, ts(0, 1, 2, 3, 4), "01234", ts(1u, 3u, 0u), ts("04231", "04231", "24031"));
  add_check(a, ts(5, 6, 7), ts("24531", "24561", "24567"));

  erase_check(a, ts(0, 1, 2), "01200", ts(1u, 0u, 2u), ts("02100", "20100", "20100"));
  add_check(a, ts(5, 6, 7), ts("50100", "56100", "56700"));

  {
    using array_t = handle_array<int, 3>;
    array_t a;
    erase_check(a, ts(0, 1, 2), "012", ts(1u, 0u, 2u), ts("021", "201", "201"));
    add_check(a, ts(5, 6, 7), ts("501", "561", "567"));
  }
}

int main() {
  test1();
  return ::test_result();
}
