
#include <iostream>
#include <rako/entity_group_manager.hpp>
#include <rako/entity_manager.hpp>

#include "timer.hpp"

using namespace rako;
using std::cout;
using std::endl;

struct pos {
  int x, y;
};
struct acc {
  int x, y;
};
struct name {
  std::string name;
};
struct other {
  float x, y;
};

int main() {

  test::timer t;
  constexpr auto num_iter = 10000;

  using A = meta::list<pos, acc, name>;
  using B = meta::list<pos, other, name>;
  using C = meta::list<name>;
  using eg_t = entity_group_manager<A, B, C>;

  using X = meta::list<pos, acc, name, other>;
  using em_t = entity_manager<X>;

  eg_t eg;
  em_t em;

  ////
  t.start();
  for (int i = 0; i < num_iter; ++i) {
    auto h = eg.add(pos{}, acc{}, name{});
    (void)h;
  }
  t.stop();
  cout << "eg " << t;

  t.start();
  for (int i = 0; i < num_iter; ++i) {
    auto h = em.create();
    em.push<pos>(h, pos{});
    em.push<acc>(h, acc{});
    em.push<name>(h, name{});
  }
  t.stop();
  cout << "em " << t;

  ////
  int j = 0;
  t.start();
  eg.for_each<meta::list<pos, name>>([&j](auto& p, auto&) {
    ++p.x;
    ++p.y;
    ++j;
  });
  t.stop();
  cout << "eg " << j << " " << t;

  j = 0;
  t.start();
  em.for_each_matching<meta::list<pos, name>>([&j](auto&, auto& p, auto&) {
    ++p.x;
    ++p.y;
    ++j;
  });
  t.stop();
  cout << "em " << j << " " << t;

  ////
  auto hg = eg.add(pos{}, acc{}, name{});
  t.start();
  for (int i = 0; i < num_iter * 10; ++i) {
    auto& pg = eg.get<pos>(hg);
    ++pg.x;
  }
  t.stop();
  auto& pg = eg.get<pos>(hg);
  cout << "eg " << pg.x << " " << t;

  auto hm = em.create();
  em.push<pos>(hm, pos{});
  t.start();
  for (int i = 0; i < num_iter * 10; ++i) {
    auto& pm = em.get<pos>(hm);
    ++pm.x;
  }
  t.stop();
  auto& pm = em.get<pos>(hm);
  cout << "em " << pm.x << " " << t;

  ////
  std::uint64_t k = 0;
  std::uint64_t l = 0;
  t.start();
  eg.for_each<meta::list<eg_t::handle, pos, name>>(
    [&k, &l, &eg](auto const& h1, auto const& p, auto&) {
      eg.for_each<meta::list<eg_t::handle, pos, name>>(
        [&h1, &k, &l, &p](auto const& h2, auto const& q, auto&) {
          // if (p.x == q.x && p.y == q.y) ++l;
          (void)p;
          (void)q;
          if (h1 == h2) ++l;
          ++k;
        });
    });
  t.stop();
  cout << "eg " << k << " " << l << " " << t;
}
