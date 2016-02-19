#pragma once

#include <cstdint>
#include <vector>


namespace rako
{
  struct component_group_traits
  {
    using groupid_t = std::uint16_t;
    using index_t = std::uint32_t;
    using counter_t = std::uint32_t;
    template <typename T>
    using vector_t = std::vector<T>;
  };
}
