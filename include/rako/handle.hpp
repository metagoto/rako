#pragma once

namespace rako {
  template <typename Index, typename Counter>
  struct handle {
    using index_t = Index;
    using counter_t = Counter;
    constexpr static index_t npos = static_cast<index_t>(-1);
    constexpr static counter_t nctr = static_cast<counter_t>(0);
    handle(index_t i, counter_t c)
      : idx(i)
      , ctr(c) {}
    handle()
      : idx(npos)
      , ctr(nctr) {}
    handle(handle const&) = default;
    handle(handle&&) = default;
    handle& operator=(handle const&) = default;
    handle& operator=(handle&&) = default;
    auto index() const { return idx; }
    auto counter() const { return ctr; }
    auto valid() const { return ctr != nctr; }
    auto invalidate() { ctr = nctr; }

   private:
    index_t idx = npos;
    counter_t ctr = nctr;
  };
}  // namespace rako
