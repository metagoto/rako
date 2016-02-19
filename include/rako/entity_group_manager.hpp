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

  template <typename...>
  struct contain;
  template <typename L, typename... Ts>
  struct contain<L, meta::list<Ts...>>
    : std::integral_constant<bool, (meta::in<typename L::comp_list, Ts>::value && ...) &&
                                     L::comp_list::size() == meta::list<Ts...>::size()>
  {
  };

  template <typename...>
  struct comp_pred;
  template <typename T, typename... Ts>
  struct comp_pred<T, meta::list<Ts...>>
    : std::integral_constant<bool, (meta::in<typename T::comp_list, Ts>::value && ...)>
  {
  };


  template <typename... CGLs> // meta::list<T1,T2>, meta::list<T1,T3>
  struct entity_group_manager
  {
    using group_tuple =
      std::tuple<component_group<CGLs, component_group_traits>...>; /// TODO: traits!
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
      using cgl = meta::find_if<group_list, meta::lambda<_a, contain<_a, CL>>>;
      using cg = meta::front<cgl>;
      constexpr auto i = group_list::size() - cgl::size();
      auto h = std::get<cg>(groups).add(std::move(t));
      h.set_group(i);
      return h;
    }

    template <typename... Ts>
    auto add(std::tuple<Ts...> const& t)
    {
      using CL = meta::list<std::decay_t<Ts>...>;
      using meta::placeholders::_a;
      using cgl = meta::find_if<group_list, meta::lambda<_a, contain<_a, CL>>>;
      using cg = meta::front<cgl>;
      constexpr auto i = group_list::size() - cgl::size();
      auto h = std::get<cg>(groups).add(t);
      h.set_group(i);
      return h;
    }

    template <typename... Ts>
    auto add(std::tuple<Ts...>& t)
    {
      using CL = meta::list<std::decay_t<Ts>...>;
      using meta::placeholders::_a;
      using cgl = meta::find_if<group_list, meta::lambda<_a, contain<_a, CL>>>;
      using cg = meta::front<cgl>;
      constexpr auto i = group_list::size() - cgl::size();
      auto h = std::get<cg>(groups).add(t);
      h.set_group(i);
      return h;
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
    template <typename...>
    struct for_each_impl;
    template <typename... Cs, typename T>
    struct for_each_impl<meta::list<Cs...>, T>
    {
      template <typename Self, typename F>
      static void call(Self& self, F&& f)
      {
        (std::get<Cs>(self.groups).template for_each<T>(std::forward<F>(f)), ...);
      }
    };

    template <typename L, typename F>
    void for_each(F&& f)
    {
      using meta::placeholders::_a;
      using C = meta::filter<group_list, meta::lambda<_a, comp_pred<_a, L>>>;
      for_each_impl<C, L>::call(*this, std::forward<F>(f));
    }


//    void test()
//    {
//      tuple_apply_index(groups, [](auto& g, auto const)
//        {
//          using cl = typename std::decay_t<decltype(g)>::comp_list;
//          g.template for_each<cl>([](auto& p, auto& a, auto& n)
//            {
//              cout << p.x << " " << p.y << " " << a.x << " " << a.y << " " << n.name << "\n";
//            });
//        });
//    }
  };
}
