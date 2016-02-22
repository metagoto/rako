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
      using type = std::tuple<component_group<Is, Ts, Traits>...>;
    };
  }


  template <typename... CGLs>
  struct entity_group_manager
  {
    using self_t = entity_group_manager<CGLs...>;

    using group_tuple = typename impl::group_id_expand<component_group_traits,
      std::index_sequence_for<CGLs...>, CGLs...>::type; // TODO: traits!

    using group_index_seq = std::make_index_sequence<std::tuple_size<group_tuple>::value>;
    using group_list = meta::as_list<group_tuple>;

    using handle = typename std::tuple_element<0, group_tuple>::type::handle;


    group_tuple groups;
    std::vector<handle> killed;

    ///
    template <typename... Cs>
    struct group_for_comp
    {
      using comp = meta::list<std::decay_t<Cs>...>;
      using type = meta::front<meta::find_if<group_list,
        meta::lambda<meta::placeholders::_a, impl::is_group<meta::placeholders::_a, comp>>>>;
    };
    template <typename... Ts>
    auto add(Ts&&... ts)
    {
      return add(std::forward_as_tuple(std::forward<Ts>(ts)...));
    }
    template <typename... Ts>
    auto add(std::tuple<Ts...>&& t)
    {
      return std::get<typename group_for_comp<Ts...>::type>(groups).add(std::move(t));
    }
    template <typename... Ts>
    auto add(std::tuple<Ts...> const& t)
    {
      return std::get<typename group_for_comp<Ts...>::type>(groups).add(t);
    }
    template <typename... Ts>
    auto add(std::tuple<Ts...>& t)
    {
      return std::get<typename group_for_comp<Ts...>::type>(groups).add(t);
    }

    ///
    template <std::size_t I>
    void remove_impl(group_tuple& t, handle& h)
    {
      std::get<I>(t).erase(h);
    }
    template <std::size_t... Is>
    void remove_dispatch(handle& h, std::index_sequence<Is...>)
    {
      static constexpr decltype(&self_t::remove_impl<0>) a[] = {&self_t::remove_impl<Is>...};
      (this->*(a[h.group()]))(groups, h);
    }
    void remove(handle& h)
    {
      if (!valid(h)) return;
      constexpr auto s = std::tuple_size<group_tuple>::value;
      remove_dispatch(h, std::make_index_sequence<s>{});
    }

    ///
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

    ///
    template <std::size_t I>
    auto valid_impl(group_tuple const& t, handle const& h) const
    {
      return std::get<I>(t).valid(h);
    }
    template <std::size_t... Is>
    auto valid_dispatch(handle const& h, std::index_sequence<Is...>) const
    {
      static constexpr decltype(&self_t::valid_impl<0>) a[] = {&self_t::valid_impl<Is>...};
      return (this->*(a[h.group()]))(groups, h);
    }
    auto valid(handle const& h) const { return valid_dispatch(h, group_index_seq{}); }

    ///
    bool alive(handle const& h) const
    {
      return valid(h) && std::find(std::begin(killed), std::end(killed), h) == std::end(killed);
    }

    ///
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
    template <typename T, std::size_t I>
    auto& get_impl(group_tuple& t, handle const& h)
    {
      return *std::get<I>(t).template get<T>(h);
    }
    template <typename T, std::size_t... Is>
    auto& get_dispatch(handle const& h, std::index_sequence<Is...>)
    {
      static constexpr decltype(&self_t::get_impl<T, 0>) a[] = {&self_t::get_impl<T, Is>...};
      return (this->*(a[h.group()]))(groups, h);
    }
    template <typename T>
    auto& get(handle const& h)
    {
      return get_dispatch<T>(h, group_index_seq{});
    }

    ///
    template <typename T, std::size_t I>
    auto const& get_c_impl(group_tuple const& t, handle const& h) const
    {
      return *std::get<I>(t).template get<T>(h);
    }
    template <typename T, std::size_t... Is>
    auto const& get_dispatch(handle const& h, std::index_sequence<Is...>) const
    {
      static constexpr decltype(&self_t::get_c_impl<T, 0>) a[] = {&self_t::get_c_impl<T, Is>...};
      return (this->*(a[h.group()]))(groups, h);
    }
    template <typename T>
    auto const& get(handle const& h) const
    {
      return get_dispatch<T>(h, group_index_seq{});
    }

    ///
    template <bool, typename...>
    struct for_each_impl;
    template <bool Handle, typename... Cs, typename T>
    struct for_each_impl<Handle, meta::list<Cs...>, T>
    {
      template <typename Self, typename F>
      static void call(Self& self, F&& f)
      {
        (std::get<Cs>(self.groups).template for_each<Handle, T>(std::forward<F>(f)), ...);
      }
    };

    template <typename L, typename F>
    void for_each(F&& f)
    {
      using meta::placeholders::_a;
      using hand = meta::in<L, handle>;
      using list = meta::filter<L, meta::lambda<_a, meta::lazy::not_<std::is_same<_a, handle>>>>;
      using select = meta::filter<group_list, meta::lambda<_a, impl::is_comp<_a, list>>>;
      for_each_impl<hand{}, select, list>::call(*this, std::forward<F>(f));
    }
  };
}
