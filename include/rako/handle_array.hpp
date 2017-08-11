#pragma once

#include <array>
#include <cassert>
#include <tuple>
#include <type_traits>

namespace rako {

  template <typename Type, std::size_t Capacity>
  struct handle_array {

    using value_type = Type;

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

    std::size_t next_free_ = 0;
    std::array<item, Capacity> lookup_;

    std::size_t sz_ = 0;
    std::array<value_type, Capacity> data_;

    handle grow() {
      assert(sz_ < Capacity);
      auto i = next_free_;
      if (i == handle::npos) i = sz_;
      next_free_ = lookup_[i].next_free;
      lookup_[i].data_index = sz_;
      auto ctr = ++lookup_[i].ctr;
      lookup_[sz_].lookup_index = i;
      return handle(i, ctr);
    }

    auto size() const { return sz_; }
    auto capacity() const { return Capacity; }

    handle push(value_type const& v) {
      auto h = grow();
      data_[sz_++] = v;
      return h;
    }

    handle push(value_type&& v) {
      auto h = grow();
      data_[sz_++] = std::move(v);
      return h;
    }

    template <typename... T>
    handle emplace(T&&... v) {
      auto h = grow();
      new (&data_[sz_++]) value_type{std::forward<T>(v)...};
      return h;
    }

    value_type& get(handle h) {
      assert(valid(h));
      auto const i = h.index();
      auto const id = lookup_[i].data_index;
      return data_[id];
    }

    bool valid(handle const& h) const {
      auto const i = h.index();
      return i < lookup_.size() && h.counter() == lookup_[i].ctr && h.counter() != handle::nctr;
    }

    void erase(handle h) {
      assert(valid(h));
      auto const i = h.index();
      h.invalidate();
      // allow calling destructor?
      lookup_[i].next_free = next_free_;
      next_free_ = i;
      ++lookup_[i].ctr;  //?
      auto id = lookup_[i].data_index;
      if (--sz_ == id) return;
      std::swap(data_[id], data_[sz_]);
      auto y = lookup_[id].lookup_index;
      auto z = lookup_[sz_].lookup_index;
      std::swap(lookup_[y].data_index, lookup_[z].data_index);
      std::swap(lookup_[id].lookup_index, lookup_[sz_].lookup_index);
    }

    void clear() {
      std::fill(std::begin(lookup_), std::end(lookup_), item{});
      std::fill(std::begin(data_), std::end(data_), value_type{});
      sz_ = 0;
      next_free_ = 0;
    }

    decltype(auto) data() { return data_.data(); }

#ifdef RAKO_TEST
    friend struct handle_array_debug;
#endif
  };

#ifdef RAKO_TEST
  struct handle_array_debug {
    template <typename T, typename F>
    void data_traverse(T const& a, F const& f) {
      for (auto const& v : a.data_) f(v);
    }
    template <typename T, typename F>
    void lookup_traverse(T const& a, F const& f) {
      for (auto const& v : a.lookup_) f(v);
    }
  };
#endif
}
