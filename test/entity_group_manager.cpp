
#ifdef __clang__
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
#include <rako/entity_group_manager.hpp>
#include <string>
#include "./simple_test.hpp"
#include "./timer.hpp"

using namespace rako;
using std::cout;
using std::endl;

struct pos {
  int x, y;
};
struct vel {
  int x, y;
};
struct name {
  std::string name;
};

auto test_for_each_handle() {

  using A = meta::list<pos, vel>;
  using B = meta::list<pos, name>;

  using em_t = entity_group_manager<A, B>;
  using handle = em_t::handle;

  em_t em;

  std::vector<handle> v1 = {em.add(pos{1, 2}, vel{3, 4}), em.add(pos{11, 22}, vel{33, 44}),
                            em.add(pos{6, 7}, name{"hello"})};

  CHECK(v1.size() == 3u);

  std::vector<handle> v2;
  em.for_each<meta::list<pos>>([&](auto&) { v2.push_back(handle()); });
  CHECK(v1.size() == v2.size());
  v2.clear();

  em.for_each<meta::list<handle, pos>>([&](auto h, auto&) { v2.push_back(h); });

  CHECK(v1.size() == v2.size());
  CHECK(v1 == v2);
}

auto test_kill() {
  using A = meta::list<pos, vel>;
  using B = meta::list<pos, name>;

  using em_t = entity_group_manager<A, B>;
  using handle = em_t::handle;

  em_t em;
  CHECK(em.size() == 0u);

  auto h1 = em.add(pos{1, 2}, vel{3, 4});
  /*auto h2 = */ em.add(pos{11, 22}, vel{33, 44});
  /*auto h3 = */ em.add(pos{6, 7}, name{"hello"});

  std::vector<handle> v1;
  em.for_each<meta::list<handle, pos>>([&](auto h, auto&) { v1.push_back(h); });
  CHECK(v1.size() == 3u);
  CHECK(em.size() == 3u);
  v1.clear();

  em.kill(h1);
  em.for_each<meta::list<handle, pos>>([&](auto h, auto&) { v1.push_back(h); });
  CHECK(v1.size() == 3u);
  v1.clear();

  em.reclaim();

  em.for_each<meta::list<handle, pos>>([&](auto h, auto&) { v1.push_back(h); });
  CHECK(v1.size() == 2u);
  CHECK(em.size() == 2u);
  v1.clear();
}

int main() {
  test_for_each_handle();
  test_kill();

  return ::test_result();
}
