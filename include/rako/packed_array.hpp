#pragma once

#include <cassert>
#include <tuple>
#include <type_traits>
#include <rako/packed_array_traits.hpp>
#include <rako/handle.hpp>

namespace rako
{
  template <typename T, typename Traits = packed_array_traits>
  struct packed_array
  {
    using self_t = packed_array<T, Traits>;
    using value_type = T;

    using index_t = typename Traits::index_t;
    using counter_t = typename Traits::counter_t;

    using handle_t = handle<index_t, counter_t>;
    constexpr static auto npos = handle_t::npos;
    constexpr static auto nctr = handle_t::nctr;

    struct item
    {
      index_t idx = npos;
      counter_t ctr = nctr;
      index_t next_free = npos;
      index_t table_idx = npos;
    };

    using handle = handle_t;
    using item_array_t = typename Traits::template vector_t<item>;
    using data_array_t = typename Traits::template vector_t<value_type>;
    item_array_t item_array;
    data_array_t data_array;
    index_t next_free = npos;
    index_t sz = 0;
    counter_t ctr = 1;

    auto size() const { return sz; }

    auto* data() { return data_array.data(); }
    auto const* data() const { return data_array.data(); }

    auto valid(handle const& h) const
    {
      if (h.counter() == nctr) return false;
      auto i = h.index();
      return i < item_array.size() && h.counter() == item_array[i].ctr;
    }

    auto next_free_idx()
    {
      if (next_free == npos) {
        auto s = item_array.size();
        item_array.resize(s + 1);
        data_array.resize(s + 1);
        return static_cast<index_t>(s);
      }
      return next_free;
    }

    auto grow()
    {
      auto i = next_free_idx();
      assert(item_array[i].ctr == nctr);
      next_free = item_array[i].next_free;
      auto ct = ctr++;
      item_array[i].ctr = ct;
      item_array[i].idx = sz;
      item_array[sz].table_idx = i;
      return handle(i, ct);
    }

    template <typename... Args>
    auto emplace(Args&&... args)
    {
      auto h = grow();
      new (&data_array[sz++]) value_type(std::forward<Args>(args)...);
      return h;
    }

    auto push(value_type&& v)
    {
      auto h = grow();
      data_array[sz++] = std::move(v);
      return h;
    }
    auto push(value_type const& v)
    {
      auto h = grow();
      data_array[sz++] = v;
      return h;
    }


    auto erase(handle& h) // erase handle... h
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

      std::swap(data_array[d], data_array[sz]);

      auto y = item_array[d].table_idx;
      auto z = item_array[sz].table_idx;
      std::swap(item_array[y].idx, item_array[z].idx);
      std::swap(item_array[d].table_idx, item_array[sz].table_idx);
    }

    auto const& get(handle const& h) const
    {
      auto i = h.index();
      assert(i < item_array.size());
      assert(h.counter() == item_array[i].ctr && h.counter() != nctr);
      return data_array[item_array[i].idx];
    }
    auto& get(handle const& h)
    {
      auto i = h.index();
      assert(i < item_array.size());
      assert(h.counter() == item_array[i].ctr && h.counter() != nctr);
      return data_array[item_array[i].idx];
    }
  };
}
