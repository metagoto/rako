#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace rako {

  template <class F, class... Ts, std::size_t... Is>
  void for_each_tuple(std::tuple<Ts...>& tup, F f, std::index_sequence<Is...>) {
    using expander = int[];
    (void)expander{0, ((void)f(std::get<Is>(tup), Is), 0)...};
  }

  template <class F, class... Ts>
  void for_each_tuple(std::tuple<Ts...>& tup, F f) {
    for_each_tuple(tup, f, std::make_index_sequence<sizeof...(Ts)>());
  }
}  // namespace rako
