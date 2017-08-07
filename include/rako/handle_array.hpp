#pragma once

#include <array>
#include <cassert>
#include <tuple>
#include <type_traits>

namespace rako {

  template <typename T, std::size_t CAPACITY>
  struct handle_array {

    using value_type = T;

    struct handle {
      constexpr static std::size_t npos = static_cast<std::size_t>(-1);
      constexpr static std::size_t nctr = static_cast<std::size_t>(0);
      std::size_t idx = npos;
      std::size_t ctr = nctr;
      handle()
        : idx(npos)
        , ctr(nctr) {}
      handle(std::size_t i, std::size_t c)
        : idx(i)
        , ctr(c) {}
      handle(handle const&) = default;
      handle(handle&&) = default;
      handle& operator=(handle const&) = default;
      handle& operator=(handle&&) = default;
      auto index() const { return idx; }
      auto counter() const { return ctr; }
      auto invalidate() {
        idx = npos;
        ctr = nctr;
      }
    };

    struct item {
      std::size_t data_index = 0;
      std::size_t lookup_index = 0;
      std::size_t ctr = handle::nctr;
      std::size_t next_free = handle::npos;
    };

    std::size_t next_free = 0;
    std::array<item, CAPACITY> lookup;

    std::size_t sz = 0;
    std::array<T, CAPACITY> data;

    handle grow() {
      assert(sz < CAPACITY);

      auto i = next_free;
      if (i == handle::npos) i = sz;

      next_free = lookup[i].next_free;
      lookup[i].data_index = sz;
      auto ctr = ++lookup[i].ctr;

      lookup[sz].lookup_index = i;

      return handle(i, ctr);
    }

    auto size() const { return sz; }

    handle add(T const& v) {
      auto h = grow();
      data[sz++] = v;
      return h;
    }

    bool valid(handle const& h) {
      auto const i = h.index();
      return i < lookup.size() && h.counter() == lookup[i].ctr && h.counter() != handle::nctr;
    }

    void erase(handle h) {
      auto const i = h.index();
      assert(valid(h));

      h.invalidate();

      lookup[i].next_free = next_free;
      next_free = i;

      ++lookup[i].ctr;  //?

      auto id = lookup[i].data_index;
      if (--sz == id) return;
      std::swap(data[id], data[sz]);

      auto y = lookup[id].lookup_index;
      auto z = lookup[sz].lookup_index;
      std::swap(lookup[y].data_index, lookup[z].data_index);
      std::swap(lookup[id].lookup_index, lookup[sz].lookup_index);
    }

    void clear() {
      std::fill(std::begin(lookup), std::end(lookup), item{});
      std::fill(std::begin(data), std::end(data), value_type{});
      sz = 0;
      next_free = 0;
    }

#ifndef NDEBUG
    friend struct handle_array_debug;
#endif
  };
}  // namespace rako

#ifndef NDEBUG
namespace rako {
  struct handle_array_debug {
    template <typename T, typename F>
    void data_traverse(T const& a, F const& f) {
      for (auto const& v : a.data) f(v);
    }
    template <typename T, typename F>
    void lookup_traverse(T const& a, F const& f) {
      for (auto const& v : a.lookup) f(v);
    }
  };
}  // namespace rako
#endif
