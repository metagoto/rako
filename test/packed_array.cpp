
#include "./simple_test.hpp"
#include <rako/packed_array.hpp>


using namespace rako;
using std::cout;
using std::endl;

struct pos
{
  pos()
    : x(0)
    , y(0)
  {
  }
  pos(int x, int y)
    : x(x)
    , y(y)
  {
  }
  pos(pos const&) = default;
  pos(pos&&) = default;
  pos& operator=(pos const&) = default;
  pos& operator=(pos&&) = default;

  int x, y;
};

//
using array_t = packed_array<pos>;

auto test_constructor()
{
  array_t a;
  CHECK(a.size() == 0u);

  auto h = a.emplace(1, 2);
  CHECK(a.size() == 1u);
  CHECK(h.index() == 0u);
  CHECK(h.counter() != 0u);

  a.erase(h);
  CHECK(a.size() == 0u);
}

auto test_data_array1()
{
  using a_t = packed_array<int>;
  a_t a;

  auto h0 = a.emplace(0);
  auto h1 = a.emplace(1);
  auto h2 = a.emplace(2);
  CHECK(a.size() == 3u);
  CHECK(a.valid(h0));
  CHECK(a.valid(h1));
  CHECK(a.valid(h2));

  using d_t = decltype(a.data_array);

  CHECK((d_t({0, 1, 2}) == a.data_array));

  a.erase(h0);
  CHECK((d_t({2, 1, 0}) == a.data_array));
  CHECK(a.size() == 2u);
  CHECK(!a.valid(h0));

  a.erase(h2);
  CHECK((d_t({1, 2, 0}) == a.data_array));
  CHECK(a.size() == 1u);
  CHECK(!a.valid(h2));

  a.erase(h1);
  CHECK((d_t({1, 2, 0}) == a.data_array));
  CHECK(a.size() == 0u);
  CHECK(!a.valid(h1));

  auto h10 = a.emplace(10);
  CHECK((d_t({10, 2, 0}) == a.data_array));
  CHECK(a.size() == 1u);

  auto h11 = a.emplace(11);
  CHECK((d_t({10, 11, 0}) == a.data_array));
  CHECK(a.size() == 2u);

  auto h12 = a.emplace(12);
  CHECK((d_t({10, 11, 12}) == a.data_array));
  CHECK(a.size() == 3u);

  auto h13 = a.emplace(13);
  CHECK((d_t({10, 11, 12, 13}) == a.data_array));
  CHECK(a.size() == 4u);

  a.erase(h13);
  CHECK((d_t({10, 11, 12, 13}) == a.data_array));
  CHECK(a.size() == 3u);

  a.erase(h12);
  CHECK((d_t({10, 11, 12, 13}) == a.data_array));
  CHECK(a.size() == 2u);

  a.erase(h10);
  CHECK((d_t({11, 10, 12, 13}) == a.data_array));
  CHECK(a.size() == 1u);

  a.erase(h11);
  CHECK((d_t({11, 10, 12, 13}) == a.data_array));
  CHECK(a.size() == 0u);

  CHECK(!a.valid(h13));
}

auto test_data_array2()
{
  using ar_t = packed_array<int>;
  using arv_t = typename ar_t::data_array_t;

  ar_t ar;
  arv_t arv = {};

#define CHECKA CHECK(ar.data_array == arv)

  CHECKA;

  auto h0 = ar.emplace(0);
  CHECK(ar.valid(h0));

  arv = {0};
  CHECKA;

  auto h1 = ar.push(1);
  CHECK(ar.valid(h1));

  auto h2 = ar.push(2);
  CHECK(ar.valid(h2));

  arv = {0, 1, 2};
  CHECKA;

  ar.erase(h2);
  CHECK(!ar.valid(h2));

  h2 = ar.emplace(22);
  CHECK(ar.valid(h2));

  ar.erase(h2);
  CHECK(!ar.valid(h2));

  CHECK(ar.valid(h0));
  ar.erase(h0);
  CHECK(!ar.valid(h0));

  ar.erase(h1);
  CHECK(!ar.valid(h1));

  int zz = 222;
  h2 = ar.push(zz);
  CHECK(ar.valid(h2));

  int const zzz = 11;
  h1 = ar.push(zzz);
  CHECK(ar.valid(h1));

  h0 = ar.emplace(0);
  CHECK(ar.valid(h0));

  auto h3 = ar.emplace(3);
  CHECK(ar.valid(h3));

  ar.erase(h1);
  CHECK(!ar.valid(h1));

  auto h4 = ar.push(4);
  CHECK(ar.valid(h4));

  arv = {222, 3, 0, 4};
  CHECKA;

#undef CHECKA
}


int main()
{

  test_constructor();
  test_data_array1();
  test_data_array2();

  return ::test_result();
}
