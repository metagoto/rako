#pragma once

#include <cassert>
#include <meta/meta.hpp>
#include <rako/group_handle.hpp>
#include <tuple>
#include <type_traits>

namespace rako {

  template <std::size_t, typename...>
  struct component_group;

  template <std::size_t ID, typename... Components, typename Traits>
  struct component_group<ID, meta::list<Components...>, Traits> {
    using self_t = component_group<ID, Components..., Traits>;

    using index_t = typename Traits::index_t;
    using counter_t = typename Traits::counter_t;
    using groupid_t = typename Traits::groupid_t;

    using handle_t = group_handle<index_t, counter_t, groupid_t>;
    constexpr static auto npos = handle_t::npos;
    constexpr static auto nctr = handle_t::nctr;
    constexpr static auto ngrp = handle_t::ngrp;
    constexpr static auto self_id = static_cast<groupid_t>(ID);

    using comp_list = meta::list<Components...>;
    template <typename U>
    using vector_t = typename Traits::template vector_t<U>;
    using comp_storage_tuple = std::tuple<vector_t<Components>...>;

    comp_storage_tuple components;

    struct item {
      index_t idx = npos;
      counter_t ctr = nctr;
      index_t next_free = npos;
      index_t table_idx = npos;
    };

    using handle = handle_t;
    using item_array_t = vector_t<item>;
    item_array_t item_array;
    index_t next_free = npos;
    index_t sz = 0;
    counter_t ctr = 1;

    auto size() const { return sz; }

    // auto* data() { return data_array.data(); }
    // auto const* data() const { return data_array.data(); }

    auto next_free_idx() {
      if (next_free == npos) {
        auto s = item_array.size();
        item_array.resize(s + 1);
        (std::get<vector_t<Components>>(components).resize(s + 1), ...);
        return static_cast<index_t>(s);
      }
      return next_free;
    }

    auto grow() {
      auto i = next_free_idx();
      assert(item_array[i].ctr == nctr);
      next_free = item_array[i].next_free;
      auto ct = ctr++;
      item_array[i].ctr = ct;
      item_array[i].idx = sz;
      item_array[sz].table_idx = i;
      return handle(i, ct, self_id);
    }

    template <typename... Ts>
    auto add(std::tuple<Ts...>&& t) {
      auto h = grow();
      (std::get<vector_t<std::decay_t<Ts>>>(components)
         .insert(std::next(std::get<vector_t<std::decay_t<Ts>>>(components).begin(), sz),
                 std::move(std::get<Ts>(t))),
       ...);
      ++sz;
      return h;
    }

    template <typename... Ts>
    auto add(std::tuple<Ts...> const& t) {
      auto h = grow();
      (std::get<vector_t<std::decay_t<Ts>>>(components)
         .insert(std::next(std::get<vector_t<std::decay_t<Ts>>>(components).begin(), sz),
                 std::get<Ts>(t)),
       ...);
      ++sz;
      return h;
    }

    auto erase(handle& h)  // handle... h
    {
      auto i = h.index();
      assert(i < item_array.size());
      assert(h.counter() == item_array[i].ctr && h.counter() != nctr);
      h.invalidate();

      item_array[i].next_free = next_free;
      item_array[i].ctr = nctr;
      next_free = i;

      auto d = item_array[i].idx;
      if (--sz == d) return;

      (std::swap(std::get<vector_t<Components>>(components)[d],
                 std::get<vector_t<Components>>(components)[sz]),
       ...);

      auto y = item_array[d].table_idx;
      auto z = item_array[sz].table_idx;
      std::swap(item_array[y].idx, item_array[z].idx);
      std::swap(item_array[d].table_idx, item_array[sz].table_idx);
    }

    template <typename T>
    std::enable_if_t<!meta::in<comp_list, T>::value, T*> get(handle const&) {
      return nullptr;
    }

    template <typename T>
    std::enable_if_t<meta::in<comp_list, T>::value, T*> get(handle const& h) {
      auto i = h.index();
      assert(i < item_array.size());
      assert(h.counter() == item_array[i].ctr && h.counter() != nctr);
      return &std::get<vector_t<T>>(components)[item_array[i].idx];
    }

    template <typename T>
    std::enable_if_t<!meta::in<comp_list, T>::value, T const*> get(handle const&) const {
      return nullptr;
    }

    template <typename T>
    std::enable_if_t<meta::in<comp_list, T>::value, T const*> get(handle const& h) const {
      auto i = h.index();
      assert(i < item_array.size());
      assert(h.counter() == item_array[i].ctr && h.counter() != nctr);
      return &std::get<vector_t<T>>(components)[item_array[i].idx];
    }

    bool valid(handle const& h) const {
      if (h.counter() == nctr) return false;
      auto i = h.index();
      return i < item_array.size() && h.counter() == item_array[i].ctr;
    }

    ///
    template <bool H, typename...>
    struct for_each_impl;
    template <typename... Ts>
    struct for_each_impl<true, meta::list<Ts...>> {
      template <typename Self, typename F>
      static void call(Self& self, F&& f) {
        for (std::size_t i = 0; i < self.sz; ++i) {
          auto id = self.item_array[i].table_idx;
          auto idx = self.item_array[id].idx;
          auto cnt = self.item_array[id].ctr;
          std::forward<F>(f)(handle(idx, cnt, self_id),
                             std::get<vector_t<Ts>>(self.components)[i]...);
        }
      }
    };
    template <typename... Ts>
    struct for_each_impl<false, meta::list<Ts...>> {
      template <typename Self, typename F>
      static void call(Self& self, F&& f) {
        for (std::size_t i = 0; i < self.sz; ++i) {
          std::forward<F>(f)(std::get<vector_t<Ts>>(self.components)[i]...);
        }
      }
    };

    template <bool H, typename L, typename F>
    void for_each(F&& f) {
      for_each_impl<H, L>::call(*this, std::forward<F>(f));
    }
  };
}  // namespace rako
