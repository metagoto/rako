#pragma once

#include <tuple>
#include <array>
#include <utility>
#include <bitset>
#include <iterator>
#include <algorithm>

#include <rako/component_group_traits.hpp>
#include <rako/component_group.hpp>

namespace rako
{

  namespace impl
  {
    template <typename...>
    struct is_group;
    template <typename L, typename... Ts>
    struct is_group<L, meta::list<Ts...>>
      : std::integral_constant<bool, (meta::in<typename L::comp_list, Ts>::value && ...) &&
                                       L::comp_list::size() == meta::list<Ts...>::size()>
    {
    };

    template <typename...>
    struct is_comp;
    template <typename T, typename... Ts>
    struct is_comp<T, meta::list<Ts...>>
      : std::integral_constant<bool, (meta::in<typename T::comp_list, Ts>::value && ...)>
    {
    };

    template <typename...>
    struct group_id_expand;
    template <typename Traits, std::size_t... Is, typename... Ts>
    struct group_id_expand<Traits, std::index_sequence<Is...>, Ts...>
    {
      using type = std::tuple<component_group<Is, Ts, component_group_traits>...>;
    };
  }



  template <typename... CGLs> // meta::list<T1,T2>, meta::list<T1,T3>, ...
  struct entity_group_manager
  {
    using group_tuple = typename impl::group_id_expand<component_group_traits,
      std::index_sequence_for<CGLs...>, CGLs...>::type; // TODO: traits!

    using group_list = meta::as_list<group_tuple>;

    using handle = typename std::tuple_element<0, group_tuple>::type::handle;

    group_tuple groups;

    std::vector<handle> killed;


    template <typename... Ts>
    auto add(Ts&&... ts)
    {
      return add(std::forward_as_tuple(std::forward<Ts>(ts)...));
    }

    template <typename... Ts>
    auto add(std::tuple<Ts...>&& t)
    {
      using CL = meta::list<std::decay_t<Ts>...>;
      using meta::placeholders::_a;
      using cgl = meta::find_if<group_list, meta::lambda<_a, impl::is_group<_a, CL>>>;
      using cg = meta::front<cgl>;
      return std::get<cg>(groups).add(std::move(t));
    }

    template <typename... Ts>
    auto add(std::tuple<Ts...> const& t)
    {
      using CL = meta::list<std::decay_t<Ts>...>;
      using meta::placeholders::_a;
      using cgl = meta::find_if<group_list, meta::lambda<_a, impl::is_group<_a, CL>>>;
      using cg = meta::front<cgl>;
      return std::get<cg>(groups).add(t);
    }

    template <typename... Ts>
    auto add(std::tuple<Ts...>& t)
    {
      using CL = meta::list<std::decay_t<Ts>...>;
      using meta::placeholders::_a;
      using cgl = meta::find_if<group_list, meta::lambda<_a, impl::is_group<_a, CL>>>;
      using cg = meta::front<cgl>;
      return std::get<cg>(groups).add(t);
    }

    ///
    template <std::size_t... Is>
    void remove_impl(handle& h, std::index_sequence<Is...>)
    {
      using p = void(group_tuple&, handle&);
      static p* a[] = {[](group_tuple& g, handle& h)
        {
          std::get<Is>(g).erase(h);
        }...};
      a[h.group()](groups, h);
    }

    void remove(handle& h)
    {
      if (!valid(h)) return;
      constexpr auto s = std::tuple_size<group_tuple>::value;
      remove_impl(h, std::make_index_sequence<s>{});
    }


    //
    void kill(handle const& h) // TODO: multiple kill
    {
      killed.push_back(h);
    }

    void reclaim()
    {
      if (killed.empty()) return;
      std::for_each(std::begin(killed), std::end(killed), [this](auto& h)
        {
          remove(h);
        });
      killed.clear();
    }

    //
    template <std::size_t... Is>
    bool valid_impl(handle const& h, std::index_sequence<Is...>) const
    {
      using p = bool(group_tuple const&, handle const&);
      static p* a[] = {[](group_tuple const& g, handle const& h) -> bool
        {
          return std::get<Is>(g).valid(h);
        }...};
      return a[h.group()](groups, h);
    }

    bool valid(handle const& h) const
    {
      constexpr auto s = std::tuple_size<group_tuple>::value;
      return valid_impl(h, std::make_index_sequence<s>{});
    }

    bool alive(handle const& h) const
    {
      return valid(h) && std::find(std::begin(killed), std::end(killed), h) == std::end(killed);
    }


    //  template <typename T = void>
    //  std::enable_if_t<std::is_same<T, void>::value, std::size_t> size() const
    //  {
    //    return entity.size();
    //  }
    template <typename...>
    struct size_impl;
    template <typename... Ts>
    struct size_impl<std::tuple<Ts...>>
    {
      template <typename Self>
      static auto call(Self const& self)
      {
        return (std::get<Ts>(self.groups).size() + ...);
      }
    };
    auto size() const { return size_impl<group_tuple>::call(*this); }


    ///
    template <typename T, std::size_t... Is>
    auto& get_impl(handle const& h, std::index_sequence<Is...>)
    {
      using p = T*(group_tuple&, handle const&);
      static p* a[] = {[](group_tuple& g, handle const& h) -> T*
        {
          return std::get<Is>(g).template get<T>(h);
        }...};
      return *a[h.group()](groups, h);
    }

    template <typename T>
    auto& get(handle const& h)
    {
      constexpr auto s = std::tuple_size<group_tuple>::value;
      return get_impl<T>(h, std::make_index_sequence<s>{});
    }

    template <typename T, std::size_t... Is>
    auto const& get_impl(handle const& h, std::index_sequence<Is...>) const
    {
      using p = T const*(group_tuple const&, handle const&);
      static p* a[] = {[](group_tuple const& g, handle const& h) -> T const*
        {
          return std::get<Is>(g).template get<T>(h);
        }...};
      return *a[h.group()](groups, h);
    }

    template <typename T>
    auto const& get(handle const& h) const
    {
      constexpr auto s = std::tuple_size<group_tuple>::value;
      return get_impl<T>(h, std::make_index_sequence<s>{});
    }


    ///
    template <bool, typename...>
    struct for_each_impl;
    template <bool H, typename... Cs, typename T>
    struct for_each_impl<H, meta::list<Cs...>, T>
    {
      template <typename Self, typename F>
      static void call(Self& self, F&& f)
      {
        (std::get<Cs>(self.groups).template for_each<H, T>(std::forward<F>(f)), ...);
      }
    };

    template <typename L, typename F>
    void for_each(F&& f)
    {
      using meta::placeholders::_a;
      using H = meta::in<L, handle>;
      using LL = meta::filter<L, meta::lambda<_a, meta::lazy::not_<std::is_same<_a, handle>>>>;
      using C = meta::filter<group_list, meta::lambda<_a, impl::is_comp<_a, LL>>>;
      for_each_impl<H{}, C, LL>::call(*this, std::forward<F>(f));
    }
  };
}
