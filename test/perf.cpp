
#include <iostream>
#include <rako/packed_array.hpp>

#include "timer.hpp"

using namespace rako;
using std::cout;
using std::endl;

struct pos {
  pos()
    : x(0)
    , y(0) {}
  pos(int x, int y)
    : x(x)
    , y(y) {}
  pos(pos const&) = default;
  pos(pos&&) = default;
  pos& operator=(pos const&) = default;
  pos& operator=(pos&&) = default;
  int x, y;
};

int main() {

  test::timer t;
  constexpr auto num_iter = 10000000;

  using ia_t = packed_array<int>;
  ia_t ia;

  t.start();
  for (int i = 0; i < num_iter; ++i) ia.emplace(i);

  t.stop();
  cout << t.ms() << " ms, size: " << ia.size() << "\n";

  using pa_t = packed_array<pos>;
  pa_t pa;

  t.start();
  for (int i = 0; i < num_iter; ++i) { pa.emplace(i, i); }
  t.stop();
  cout << t.ms() << " ms, size: " << pa.size() << "\n";

  t.start();
  for (unsigned int i = 0u, n = pa.size(); i != n; ++i) { pa.data_array.data()[i].x += 1; }
  t.stop();
  cout << t.ms() << " ms, size: " << pa.size() << "\n";

  cout << "shoudl be 3: " << (pa.data_array[2].x) << "\n";

  return 0;
}
