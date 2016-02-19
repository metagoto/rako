#pragma once

namespace rako
{
  template <typename Index, typename Counter, typename GroupId>
  struct group_handle
  {
    using index_t = Index;
    using counter_t = Counter;
    using groupid_t = GroupId;
    constexpr static index_t npos = static_cast<index_t>(-1);
    constexpr static counter_t nctr = static_cast<counter_t>(0);
    constexpr static groupid_t ngrp = static_cast<groupid_t>(-1);
    group_handle(index_t i, counter_t c, groupid_t g /* = ngrp*/)
      : idx(i)
      , ctr(c)
      , gid(g)
    {
    }
    group_handle()
      : idx(npos)
      , ctr(nctr)
      , gid(ngrp)
    {
    }
    group_handle(group_handle const&) = default;
    group_handle(group_handle&&) = default;
    group_handle& operator=(group_handle const&) = default;
    group_handle& operator=(group_handle&&) = default;
    auto index() const { return idx; }
    auto counter() const { return ctr; }
    auto group() const { return gid; }
    auto valid() const { return ctr != nctr; }
    auto invalidate() { ctr = nctr; }
  private:
    // auto set_group(groupid_t g) { gid = g; }
    index_t idx = npos;
    counter_t ctr = nctr;
    groupid_t gid = ngrp;
    // template <typename...> friend class entity_group_manager;

    friend bool operator==(group_handle const& a, group_handle const& b)
    {
      return a.index() == b.index() && a.counter() == b.counter() && a.group() == b.group();
    }
    friend bool operator!=(group_handle const& a, group_handle const& b) { return !(a == b); }
  };
}
