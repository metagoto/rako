
#ifdef __clang__
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
#include <rako/entity_manager.hpp>
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

  using em_t = entity_manager<A, B>;
  using handle = em_t::handle;

  em_t em;

  std::vector<handle> v1 = {em.add(pos{1, 2}, vel{3, 4}), em.add(pos{11, 22}, vel{33, 44}),
                            em.add(pos{6, 7}, name{"hello"})};

  CHECK(v1.size() == 3u);

  std::vector<handle> v2;
  em.for_each<pos>([&](auto&) { v2.push_back(handle()); });
  CHECK(v1.size() == v2.size());
  v2.clear();

  em.for_each<handle, pos>([&](auto h, auto&) { v2.push_back(h); });

  CHECK(v1.size() == v2.size());
  CHECK(v1 == v2);
}

auto test_kill() {
  using A = meta::list<pos, vel>;
  using B = meta::list<pos, name>;

  using em_t = entity_manager<A, B>;
  using handle = em_t::handle;

  em_t em;
  CHECK(em.size() == 0u);

  auto h1 = em.add(pos{1, 2}, vel{3, 4});
  /*auto h2 = */ em.add(pos{11, 22}, vel{33, 44});
  /*auto h3 = */ em.add(pos{6, 7}, name{"hello"});

  std::vector<handle> v1;
  em.for_each<handle, pos>([&](auto h, auto&) { v1.push_back(h); });
  CHECK(v1.size() == 3u);
  CHECK(em.size() == 3u);
  v1.clear();

  em.kill(h1);
  em.for_each<handle, pos>([&](auto h, auto&) { v1.push_back(h); });
  CHECK(v1.size() == 3u);
  v1.clear();

  em.reclaim();

  em.for_each<handle, pos>([&](auto h, auto&) { v1.push_back(h); });
  CHECK(v1.size() == 2u);
  CHECK(em.size() == 2u);
  v1.clear();
}

auto test_struct_binding() {
  using A = meta::list<pos, vel>;
  using B = meta::list<pos, name>;

  using em_t = entity_manager<A, B>;
  // using handle = em_t::handle;

  em_t em;

  auto h1 = em.add(pos{1, 2}, vel{3, 4});
  /*auto h2 = */ em.add(pos{11, 22}, vel{33, 44});
  auto h3 = em.add(pos{6, 7}, name{"hello"});

  auto p = em.get<pos>(h1);
  ++p.x;
  CHECK((p.x == 2 && p.y == 2));
  p = em.get<pos>(h1);
  CHECK((p.x == 1 && p.y == 2));

  auto& pp = em.get<pos>(h1);
  ++pp.x;
  CHECK((pp.x == 2 && pp.y == 2));
  p = em.get<pos>(h1);
  CHECK((p.x == 2 && p.y == 2));

  auto v = em.get<vel>(h1);
  CHECK((v.x == 3 && v.y == 4));

  auto[ps, vs] = em.get<pos, vel>(h1);
  ++ps.x;
  ++vs.y;
  CHECK((ps.x == 3 && ps.y == 2 && vs.x == 3 && vs.y == 5));

  auto[ps2, vs2] = em.get<pos, vel>(h1);
  CHECK((ps2.x == 3 && ps2.y == 2 && vs2.x == 3 && vs2.y == 5));

  auto p2 = em.get<pos>(h3);
  CHECK((p2.x == 6 && p2.y == 7));

  auto[ps3, ns3] = em.get<meta::list<pos, name>>(h3);
  CHECK((ps3.x == 6 && ps3.y == 7 && ns3.name.size() == 5));

  auto& pp2 = em.get<meta::list<pos>>(h3);
  ++pp2.x;
  CHECK((pp2.x == 7 && pp2.y == 7));
  CHECK(em.get<meta::list<pos>>(h3).x == 7);

  [h1, h3](em_t const& em) {
    auto p = em.get<pos>(h1);
    CHECK((p.x == 3 && p.y == 2));
    auto v = em.get<vel>(h1);
    CHECK((v.x == 3 && v.y == 5));

    auto[ps, vs] = em.get<pos, vel>(h1);
    CHECK((ps.x == 3 && ps.y == 2 && vs.x == 3 && vs.y == 5));

    auto const[ps2, vs2] = em.get<pos, vel>(h1);
    CHECK((ps2.x == 3 && ps2.y == 2 && vs2.x == 3 && vs2.y == 5));

    auto p2 = em.get<pos>(h3);
    CHECK((p2.x == 7 && p2.y == 7));

    auto[ps3, ns3] = em.get<meta::list<pos, name>>(h3);
    CHECK((ps3.x == 7 && ps3.y == 7 && ns3.name.size() == 5));

    auto& pp2 = em.get<meta::list<pos>>(h3);
    //++pp2.x;
    CHECK((pp2.x == 7 && pp2.y == 7));
  }(em);
}

decltype(auto) ft() {
  static auto x = 0;
  static auto y = 0;
  return std::tuple<int&, int&>{x, y};  // std::make_tuple(x, y);
}
auto test_struct_binding2() {
  auto[x, y] = ft();
  CHECK((x == 0 && y == 0));
  ++x;
  ++y;
  auto[x2, y2] = ft();
  CHECK((x2 == 1 && y2 == 1));

  auto[x3, y3] = ft();
  CHECK((x3 == 1 && y3 == 1));
  ++x;
  ++y;
  auto[x4, y4] = ft();
  CHECK((x4 == 2 && y4 == 2));
}

auto test_groups() {
  using A = meta::list<pos, vel>;
  using B = meta::list<pos, vel>;
  using C = meta::list<pos, vel, name>;
  using em_t = entity_manager<A, B, C>;
  em_t em;

  /*auto h1 = */ em.add(pos{1, 2}, vel{3, 4});
  /*auto h2 = */ em.add(pos{11, 22}, vel{33, 44});
  /*auto h3 = */ em.add(pos{6, 7}, vel{42, 43}, name{"hello"});

  CHECK(true);
}

template <typename T>
struct identity {
  using type = T;
};

template <typename... T>
void each(typename identity<std::function<void(T&...)>>::type f) {
  static int i = 0;
  static float j = 1.f;
  f(i, j);
}

auto test_each() {
  each<int, float>([](auto, auto const&) { CHECK(true); });
}

int main() {
  test_for_each_handle();
  test_kill();
  test_struct_binding();
  test_struct_binding2();

  test_groups();
  test_each();

  return ::test_result();
}
