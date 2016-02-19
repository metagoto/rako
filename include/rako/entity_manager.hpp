#pragma once

#include <meta/meta.hpp>

#include <tuple>
#include <array>
#include <utility>
#include <bitset>
#include <iterator>
#include <algorithm>

#include <rako/packed_array.hpp>

namespace rako
{

  namespace impl
  {
    template <typename...>
    struct tuple_array_expand;
    template <typename Traits, typename... T>
    struct tuple_array_expand<Traits, meta::list<T...>>
    {
      using type = std::tuple<packed_array<T, Traits>...>;
    };

    template <typename T, typename F, std::size_t... I>
    auto tuple_apply_index_impl(T&& t, F const& f, std::index_sequence<I...>)
    {
      return (f(std::get<I>(std::forward<T>(t)), I), ...);
    }
    template <typename T, typename F>
    auto tuple_apply_index(T&& t, F&& f)
    {
      constexpr auto s = std::tuple_size<typename std::decay<T>::type>::value;
      return tuple_apply_index_impl(
        std::forward<T>(t), std::forward<F>(f), std::make_index_sequence<s>{});
    }
  }

  template <typename Components, typename Tags = meta::list<>>
  struct entity_manager
  {
    using component_list = Components;
    using tag_list = Tags;

    static_assert(meta::unique<component_list>::size() == component_list::size());
    static_assert(meta::unique<tag_list>::size() == tag_list::size());
    using all_list = meta::concat<component_list, tag_list>;
    static_assert(meta::unique<all_list>::size() == all_list::size());

    using component_tuple_t =
      typename impl::tuple_array_expand<packed_array_traits, component_list>::type;

    using component_handle_t = typename std::tuple_element<0, component_tuple_t>::type::handle_t;

    constexpr static auto bset_size() { return 1 + component_list::size() + tag_list::size(); }
    using bitset_t = std::bitset<bset_size()>;

    struct data
    {
      bitset_t bset; // bset[0] : killed
      std::array<component_handle_t, component_list::size()> component;
    };
    using entity_array_t = packed_array<data /*, traits*/>;
    using handle = typename entity_array_t::handle;

    template <typename T>
    using index_of_comp = meta::find_index<component_list, typename std::decay<T>::type>;
    template <typename T>
    using index_of_tag = meta::find_index<tag_list, typename std::decay<T>::type>;
    template <typename T>
    using index_of_all = meta::find_index<all_list, typename std::decay<T>::type>;
    constexpr static auto bset_all_index(std::size_t i) { return 1 + i; }
    constexpr static auto bset_comp_index(std::size_t i) { return 1 + i; }
    constexpr static auto bset_tag_index(std::size_t i) { return 1 + component_list::size() + i; }

    component_tuple_t component_data;
    entity_array_t entity;
    std::vector<handle> killed;


    auto create() { return entity.emplace(); }

    auto& remove(handle& h)
    {
      if (!entity.valid(h)) return *this;
      auto& d = entity.get(h);
      impl::tuple_apply_index(component_data, [&d](auto& a, auto const& i)
        {
          auto e = d.component[i];
          if (a.valid(e)) a.erase(e);
        });
      d.bset.reset();
      entity.erase(h);
      return *this;
    }

    auto& kill(handle const& h)
    {
      if (!entity.valid(h)) return *this;
      entity.get(h).bset.set(0);
      killed.push_back(h);
      return *this;
    }

    auto& reclaim()
    {
      std::for_each(std::begin(killed), std::end(killed), [this](auto h)
        {
          remove(h);
        });
      killed.clear();
      return *this;
    }

    bool valid(handle const& h) const { return entity.valid(h); }

    bool alive(handle const& h) const { return valid(h) && !entity.get(h).bset[0]; }


    template <typename T = void>
    std::enable_if_t<std::is_same<T, void>::value, std::size_t> size() const
    {
      return entity.size();
    }

    template <typename T = void>
    std::enable_if_t<!std::is_same<T, void>::value, std::size_t> size() const
    {
      constexpr auto i = index_of_comp<T>{};
      static_assert(i != meta::npos());
      auto& a = std::get<i>(component_data);
      return a.size();
    }

    template <typename T>
    auto& push(handle const& h, T&& comp)
    {
      constexpr auto i = index_of_comp<T>{};
      static_assert(i != meta::npos());
      auto& a = std::get<i>(component_data);
      auto const& c = a.push(std::forward<T>(comp));
      auto& d = entity.get(h);
      d.bset.set(bset_comp_index(i));
      d.component[i] = c;
      return *this;
    }

    template <typename T, typename... Args>
    auto& emplace(handle const& h, Args&&... args)
    {
      constexpr auto i = index_of_comp<T>{};
      static_assert(i != meta::npos());
      auto& a = std::get<i>(component_data);
      auto const& c = a.emplace(std::forward<Args>(args)...);
      auto& d = entity.get(h);
      d.bset.set(bset_comp_index(i));
      d.component[i] = c;
      // return *this;
      return get<T>(h);
    }

    template <typename T>
    auto& erase(handle const& h)
    {
      constexpr auto i = index_of_comp<T>{};
      static_assert(i != meta::npos());
      auto& d = entity.get(h);
      assert(d.bset[bset_comp_index(i)]);
      d.bset.reset(bset_comp_index(i));
      auto const& c = d.component[i];
      auto& a = std::get<i>(component_data);
      a.erase(c);
      return *this;
    }

    template <typename T>
    auto& tag(handle const& h)
    {
      constexpr auto i = index_of_tag<T>{};
      static_assert(i != meta::npos());
      auto& d = entity.get(h);
      d.bset.set(bset_tag_index(i));
      return *this;
    }

    template <typename T>
    auto& untag(handle const& h)
    {
      constexpr auto i = index_of_tag<T>{};
      static_assert(i != meta::npos());
      auto& d = entity.get(h);
      d.bset.reset(bset_tag_index(i));
      return *this;
    }

    template <typename T>
    auto& get(handle const& h)
    {
      constexpr auto i = index_of_comp<T>{};
      static_assert(i != meta::npos());
      auto& d = entity.get(h);
      assert(d.bset[bset_comp_index(i)]);
      auto const& c = d.component[i];
      auto& a = std::get<i>(component_data);
      return a.get(c);
    }

    template <typename T>
    auto const& get(handle const& h) const
    {
      constexpr auto i = index_of_comp<T>{};
      static_assert(i != meta::npos());
      auto& d = entity.get(h);
      assert(d.bset[bset_comp_index(i)]);
      auto const& c = d.component[i];
      auto& a = std::get<i>(component_data);
      return a.get(c);
    }

    template <typename... T>
    bool has(handle const& h) const
    {
      static_assert(((index_of_all<T>{} != meta::npos()) && ...));
      auto const& d = entity.get(h);
      return (d.bset[bset_all_index(index_of_all<T>{})] && ...);
    }

    template <typename T>
    auto* data()
    {
      constexpr auto i = index_of_comp<T>{};
      static_assert(i != meta::npos());
      auto& a = std::get<i>(component_data);
      return a.data();
    }

    template <typename F>
    auto for_each(F&& f) const
    {
      for (auto const& e : entity.item_array) {
        if (e.ctr != handle::nctr) std::forward<F>(f)(handle(e.idx, e.ctr));
      }
    }

    template <typename...>
    struct for_each_matching_impl;
    template <typename... T, typename... C>
    struct for_each_matching_impl<meta::list<T...>, meta::list<C...>>
    {
      static_assert(((index_of_all<T>{} != meta::npos()) && ...));
      template <typename S, typename F, typename H>
      static auto call(S&& self, F&& f, H const& h)
      {
        auto const& d = std::forward<S>(self).entity.get(h);
        if ((d.bset[bset_all_index(index_of_all<T>{})] && ...)) {
          std::forward<F>(f)(h, std::forward<S>(self).template get<C>(h)...);
        }
      }
    };

    template <typename T>
    struct comp_pred : std::integral_constant<bool, index_of_comp<T>{} != meta::npos()>
    {
    };

    template <typename L, typename F>
    auto for_each_matching(F&& f) // TODO: merge const and non const
    {
      using C = meta::filter<L, meta::quote<comp_pred>>;
      for (auto const& e : entity.item_array) {
        if (e.ctr != handle::nctr) {
          for_each_matching_impl<L, C>::call(*this, std::forward<F>(f), handle(e.idx, e.ctr));
        }
      }
    }
    template <typename L, typename F>
    auto for_each_matching(F&& f) const
    {
      using C = meta::filter<L, meta::quote<comp_pred>>;
      for (auto const& e : entity.item_array) {
        if (e.ctr != handle::nctr) {
          for_each_matching_impl<L, C>::call(*this, std::forward<F>(f), handle(e.idx, e.ctr));
        }
      }
    }
  };
}
