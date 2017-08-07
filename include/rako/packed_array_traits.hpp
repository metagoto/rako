#pragma once

#include <cstdint>
#include <vector>

namespace rako {
  struct packed_array_traits {
    using index_t = std::uint32_t;
    using counter_t = std::uint32_t;
    template <typename T>
    using vector_t = std::vector<T>;
  };
}  // namespace rako
