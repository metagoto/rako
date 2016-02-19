#pragma once

#include <chrono>

namespace test
{
  struct timer
  {
    using clock_type = std::conditional<std::chrono::high_resolution_clock::is_steady,
      std::chrono::high_resolution_clock, std::chrono::steady_clock>::type;

    void start() { start_ = clock_type::now(); }
    void stop() { stop_ = clock_type::now(); }

    auto ms() const
    {
      return std::chrono::duration_cast<std::chrono::milliseconds>(stop_ - start_).count();
    }

    decltype(clock_type::now()) start_ = decltype(clock_type::now()){};
    decltype(clock_type::now()) stop_ = decltype(clock_type::now()){};

    template <typename T>
    friend T& operator<<(T& os, timer const& t)
    {
      os << "timer: " << t.ms() << "ms\n";
      return os;
    }
  };
}
