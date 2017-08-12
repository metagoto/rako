
#ifdef __clang__
#pragma clang diagnostic ignored "-Wfloat-conversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
#include <rako/entity_manager.hpp>
#include "./simple_test.hpp"
#include "./timer.hpp"

using namespace rako;
using std::cout;
using std::endl;

//
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

struct body {
  body()
    : x(0)
    , y(0)
    , vel(0) {}
  body(int x, int y, int v)
    : x(x)
    , y(y)
    , vel(v) {}
  body(body const&) = default;
  body(body&&) = default;
  body& operator=(body const&) = default;
  body& operator=(body&&) = default;

  int x, y, vel;
};

auto test_blah() {
  using l = meta::list<int, double, pos>;

  entity_manager<l> em;

  auto h = em.create();
  CHECK(em.valid(h));
  CHECK(em.alive(h));

  em.emplace<int>(h, 42);
  em.emplace<double>(h, 3.14);
  CHECK(em.valid(h));
  CHECK(em.alive(h));

  CHECK(em.has<int>(h));
  CHECK(em.has<double>(h));
  CHECK(!em.has<pos>(h));

  auto& ci = em.get<int>(h);
  CHECK(ci == 42);

  auto& cd = em.get<double>(h);
  CHECK(cd == 3.14);
  cd = 2.718;
  CHECK(em.get<double>(h) == 2.718);

  // auto& cp = em.get<pos>(h); // assert

  em.emplace<pos>(h, 5, 7);
  CHECK(em.has<pos>(h));
  auto& cp = em.get<pos>(h);
  CHECK((cp.x == 5 && cp.y == 7));
  cp.x += 1;
  cp.y += 2;
  auto& cpp = em.get<pos>(h);
  CHECK((cpp.x == 6 && cpp.y == 9));

  CHECK((em.has<pos, int, double>(h)));

  CHECK(em.has<double>(h));
  em.erase<double>(h);
  CHECK(!em.has<double>(h));

  CHECK(!(em.has<pos, int, double>(h)));
  CHECK((em.has<pos, int>(h)));

  em.remove(h);
  CHECK(!em.alive(h));
  CHECK(!em.valid(h));
}

auto test_blah2() {
  srand(static_cast<unsigned>(time(0)));

  using l = meta::list<body>;
  entity_manager<l> em;

  for (auto i = 0; i < 1000; ++i) {
    auto h = em.create();
    CHECK(em.valid(h));
    CHECK(em.alive(h));
    em.emplace<body>(h, rand() % 100, rand() % 100, rand() % 100);
  }
}

auto test_blah3() {
  entity_manager<meta::list<int, double, std::string>> em;

  auto z = em.create();
  CHECK(em.size() == 1ul);
  CHECK(em.alive(z));
  CHECK(em.valid(z));
  CHECK((!em.has<int>(z) && !em.has<double>(z) && !em.has<std::string>(z)));

  em.kill(z);
  CHECK(em.size() == 1ul);
  CHECK(!em.alive(z));
  CHECK(em.valid(z));

  em.remove(z);
  CHECK(em.size() == 0ul);
  CHECK(!em.alive(z));
  CHECK(!em.valid(z));

  z = em.create();
  CHECK(em.size() == 1ul);
  CHECK(em.alive(z));
  CHECK(em.valid(z));

  auto z2 = em.create();
  CHECK(em.size() == 2ul);
  CHECK(em.alive(z2));
  CHECK(em.valid(z2));

  em.kill(z);
  CHECK(em.size() == 2ul);
  CHECK(!em.alive(z));
  CHECK(em.valid(z));
  CHECK(em.alive(z2));
  CHECK(em.valid(z2));

  em.reclaim();
  CHECK(em.size() == 1ul);
  CHECK(!em.alive(z));
  CHECK(!em.valid(z));
  CHECK(em.alive(z2));
  CHECK(em.valid(z2));

  {
    CHECK(!em.has<int>(z2));
    em.emplace<int>(z2, 0);
    CHECK((em.has<int>(z2) && !em.has<double>(z2) && !em.has<std::string>(z2)));
    auto i = em.get<int>(z2);
    CHECK(i == 0);

    em.push<int>(z2, 1);
    i = em.get<int>(z2);
    CHECK(em.has<int>(z2));
    CHECK(i == 1);

    auto& j = em.get<int>(z2);
    ++j;
    auto k = em.get<int>(z2);
    CHECK(k == 2);

    em.erase<int>(z2);
    CHECK(!em.has<int>(z2));
  }

  {
    CHECK(!em.has<std::string>(z2));
    em.emplace<std::string>(z2, "hi");
    CHECK((!em.has<int>(z2) && !em.has<double>(z2) && em.has<std::string>(z2)));
    auto& i = em.get<std::string>(z2);
    CHECK(i == "hi");

    em.push<std::string>(z2, "hello");
    auto& j = em.get<std::string>(z2);
    CHECK(em.has<std::string>(z2));
    CHECK(j == "hello");

    j = "bjr";
    CHECK(em.has<std::string>(z2));
    CHECK(j == "bjr");

    em.remove(z2);
    CHECK(em.size() == 0ul);
    CHECK(!em.alive(z2));
    CHECK(!em.valid(z2));
  }
}

auto test_reclaim() {
  using l = meta::list<int, double, pos>;

  entity_manager<l> em;
  CHECK(em.size() == 0ul);

  auto h = em.create();
  CHECK(em.valid(h));
  CHECK(em.alive(h));
  CHECK(em.size() == 1ul);

  em.reclaim();
  CHECK(em.valid(h));
  CHECK(em.alive(h));
  CHECK(em.size() == 1ul);

  auto h2 = em.create();
  CHECK(em.valid(h2));
  CHECK(em.alive(h2));
  CHECK(em.size() == 2ul);

  em.kill(h);
  CHECK(em.valid(h));
  CHECK(!em.alive(h));
  CHECK(em.size() == 2ul);

  em.reclaim();
  CHECK(!em.valid(h));
  CHECK(!em.alive(h));
  CHECK(em.size() == 1ul);

  //  em.kill(h); // noop
  //  CHECK(!em.valid(h));
  //  CHECK(!em.alive(h));
  //  CHECK(em.size() == 1ul);

  em.kill(h2);
  CHECK(em.valid(h2));
  CHECK(!em.alive(h2));
  CHECK(em.size() == 1ul);

  em.reclaim();
  // em.remove(h2);
  CHECK(!em.valid(h2));
  CHECK(!em.alive(h2));
  CHECK(em.size() == 0ul);
}

auto test_reclaim2() {
  using l = meta::list<int, double, pos>;

  entity_manager<l> em;
  CHECK(em.size() == 0ul);

  auto h = em.create();
  CHECK(em.valid(h));
  CHECK(em.alive(h));
  CHECK(em.size() == 1ul);

  //  em.reclaim();
  //  CHECK(em.valid(h));
  //  CHECK(em.alive(h));
  //  CHECK(em.size() == 1ul);

  em.kill(h);
  CHECK(em.valid(h));
  CHECK(!em.alive(h));
  CHECK(em.size() == 1ul);

  em.reclaim();
  CHECK(!em.valid(h));
  CHECK(!em.alive(h));
  CHECK(em.size() == 0ul);

  auto h2 = em.create();
  CHECK(em.valid(h2));
  CHECK(em.alive(h2));
  CHECK(em.size() == 1ul);

  em.kill(h2);
  CHECK(em.valid(h2));
  CHECK(!em.alive(h2));
  CHECK(em.size() == 1ul);

  em.reclaim();
  // em.remove(h2);
  CHECK(!em.valid(h2));
  CHECK(!em.alive(h2));
  CHECK(em.size() == 0ul);

  CHECK(!em.valid(h));
  CHECK(!em.alive(h));
}

struct tag1 {};
struct tag2 {};

auto test_blah4() {
  using L = meta::list<int, body>;
  using T = meta::list<tag1, tag2>;
  {
    entity_manager<L> em;
    auto h = em.create();
    em.emplace<body>(h, 42, 53, 72);
    CHECK((!em.has<int>(h) && em.has<body>(h)));
    auto& b = em.get<body>(h);
    CHECK(b.x == 42);
    CHECK(b.y == 53);
    CHECK(++b.vel == 73);
    auto c = em.get<body>(h);
    CHECK(c.vel == 73);
  }
  {
    entity_manager<L, T> em;
    constexpr auto num_iter = 10;
    for (int i = 0; i < num_iter; ++i) {
      auto h = em.create();
      body b(i, i * 10, i * 100);
      em.push(h, b);
    }
    CHECK(em.size() == 10ul);
    auto* p = em.data<body>();
    int i = 0;
    for (; i < static_cast<int>(em.size<body>()); ++i, ++p) {
      CHECK(p->x == i);
      CHECK(p->y == i * 10);
      CHECK(p->vel == i * 100);
    }
    CHECK(i == num_iter);

    int tot = 0;
    em.for_each([&](auto e) { tot += em.get<body>(e).x; });
    CHECK(tot == 45);
    auto h = em.create();
    em.emplace<int>(h, 1);
    tot = 0;
    em.for_each([&](auto e) {
      if (em.has<body>(e)) tot += em.get<body>(e).x;
    });
    CHECK(tot == 45);
    em.emplace<body>(h, 1, 2, 3);
    tot = 0;
    em.for_each([&](auto e) { tot += em.get<body>(e).y; });
    CHECK(tot == 452);
    em.erase<body>(h);
    tot = 0;
    em.for_each([&](auto e) {
      if (em.has<body>(e)) {
        tot += em.get<body>(e).y;
        em.get<body>(e).vel += 10;
      }
    });
    CHECK(tot == 450);

    h = em.create();
    em.emplace<int>(h, 42);
    em.emplace<body>(h, 10, 20, 30);
    em.tag<tag1>(h);
    using sig = meta::list<body, int>;
    em.for_each_matching<sig>([](auto, auto& b, auto& i) {
      ++b.x;
      ++i;
    });
    CHECK(em.get<body>(h).x == 11);
    CHECK(em.get<int>(h) == 43);

    tot = 0;
    using sig2 = meta::list<tag1>;
    em.for_each_matching<sig2>([&](auto) { ++tot; });
    CHECK(tot == 1);
  }
  {
    entity_manager<L, T> em;
    auto h = em.create();
    em.emplace<int>(h, 42);
    em.emplace<body>(h, 10, 20, 30);
    em.tag<tag1>(h);
    using sig = meta::list<body, int>;

    [](auto const& e) {
      e.template for_each_matching<sig>([](auto, auto const& /*b*/, auto const& /*i*/) {});
    }(em);
  }
}

auto test_blah5() {
  using C = meta::list<int, body>;
  using T = meta::list<tag1, tag2>;

  entity_manager<C, T> em;
  auto h = em.create();
  em.tag<tag1>(h);

  CHECK(em.has<tag1>(h));
  CHECK(!em.has<tag2>(h));
  CHECK(!em.has<body>(h));

  em.emplace<body>(h, 1, 2, 3);
  CHECK(em.has<tag1, body>(h));

  CHECK(!em.has<int, tag1, body>(h));

  CHECK(em.has<tag1>(h));
  em.untag<tag1>(h);
  CHECK(!em.has<tag1>(h));
}

auto test_blah6() {
  using std::cout;
  using C = meta::list<int, body>;
  using T = meta::list<tag1, tag2>;
  using siga = meta::list<body>;

  entity_manager<C, T> em;

  constexpr auto num_iter = 100000;

  test::timer t;
  t.start();
  for (int i = 0; i < num_iter; ++i) {
    auto h = em.create();
    em.emplace<int>(h, i);
    em.emplace<body>(h, 1, 2, 3);
  }
  t.stop();
  cout << t;

  int tot = 0;
  t.start();
  em.for_each_matching<siga>([&tot](auto e, auto& b) {
    CHECK(e.valid());
    b.x += b.vel * 3;
    b.y += b.vel * 3;
    ++tot;
  });
  t.stop();
  cout << t;
  CHECK(tot == num_iter);
}

auto test_for() {}

auto test_for2() {}

int main() {
  test_blah();
  test_blah2();
  test_blah3();
  test_reclaim();
  test_reclaim2();
  test_blah4();
  test_blah5();
  test_blah6();
  test_for();
  test_for2();

  return ::test_result();
}
