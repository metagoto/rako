#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cassert>
#include <vector>

namespace rako {

  template <typename T>
  struct tmpcont {
    using cont_t = std::vector<T>;

    cont_t cont;

    void insert(T t) {
      auto i = find(t);
      if (i == cont.end()) cont.push_back(t);
    }

    void remove(T t) {
      auto i = find(t);
      if (i != cont.end()) cont.erase(i);
    }

    auto find(T t) const { return std::find(cont.begin(), cont.end(), t); }

    void clear() { cont.clear(); }

    auto size() const { return cont.size(); }

    template <typename F>
    auto test(F&& f) {
      for (auto const& p : cont) {
        auto const& nodes = p->test();
        // for (auto& data : nodes) {
        std::forward<F>(f)(nodes);
        //}
      }
    }
  };

  template <typename T, int MaxNode, int MaxLevel>
  struct qtree {
    using self_t = qtree<T, MaxNode, MaxLevel>;
    using value_type = T;

    struct data {
      T obj;
      float x, y;
    };

    static constexpr auto NW = 0;
    static constexpr auto NE = 1;
    static constexpr auto SE = 2;
    static constexpr auto SW = 3;

    qtree(float x, float y, float w, float h, int level = 0)
      : ax(x)
      , ay(y)
      , aw(w)
      , ah(h)
      , level(level)
      , leaf(true)
      , shape(sf::Vector2f(w, h)) {
      set_shape();
    }

    ////
    auto test() const {
      return nodes;  //// copy
    }

    ////
    template <typename C>
    bool insert(value_type v, float x, float y, C& cont)  // also value_type const&
    {
      if (x < ax || x > ax + aw || y < ay || y > ay + ah) return false;
      if ((nodes.size() < MaxNode && leaf) || level == MaxLevel) {
        nodes.push_back(data{std::move(v), x, y});

        if (nodes.size() > 2) cont.insert(this);  /////

        return true;
      }
      leaf = false;

      cont.remove(this);  /////

      if (!quad[0]) split(cont);

      if (quad[NW]->insert(std::move(v), x, y, cont)) return true;
      if (quad[NE]->insert(std::move(v), x, y, cont)) return true;
      if (quad[SE]->insert(std::move(v), x, y, cont)) return true;
      if (quad[SW]->insert(std::move(v), x, y, cont)) return true;
      assert(false);
      return false;
    }

    template <typename C>
    void split(C& cont) {
      quad[NW].reset(new self_t(ax, ay, aw / 2.f, ah / 2.f, level + 1));
      quad[NE].reset(new self_t(ax + aw / 2.f, ay, aw / 2.f, ah / 2.f, level + 1));
      quad[SE].reset(new self_t(ax + aw / 2.f, ay + ah / 2.f, aw / 2.f, ah / 2.f, level + 1));
      quad[SW].reset(new self_t(ax, ay + ah / 2.f, aw / 2.f, ah / 2.f, level + 1));
      std::for_each(std::begin(nodes), std::end(nodes), [this, &cont](auto data) {
        insert(std::move(data.obj), data.x, data.y, cont);
      });
      nodes.clear();
    }

    void clear() {
      if (quad[0]) {
        quad[NW]->clear();
        quad[NE]->clear();
        quad[SE]->clear();
        quad[SW]->clear();
      }
      nodes.clear();
      leaf = true;
    }

    void reset(float x, float y, float w, float h, int lev = 0) {
      if (quad[0]) {
        quad[NW].reset();
        quad[NE].reset();
        quad[SE].reset();
        quad[SW].reset();
      }
      ax = x;
      ay = y;
      aw = w;
      ah = h;
      level = lev;
      nodes.clear();
      leaf = true;
    }

    void set_shape() {
      shape.setSize(sf::Vector2f(aw, ah));
      shape.setPosition(ax, ay);
      shape.setOutlineThickness(1.f);
      shape.setOutlineColor(sf::Color::Red);
      shape.setFillColor(sf::Color::Black);
    }

    void draw(sf::RenderTarget& target) {
      target.draw(shape);
      if (!leaf && quad[0]) {
        quad[NW]->draw(target);
        quad[NE]->draw(target);
        quad[SE]->draw(target);
        quad[SW]->draw(target);
      }
    }

    std::array<std::unique_ptr<self_t>, 4> quad;
    std::vector<data> nodes;

    float ax, ay, aw, ah;
    int level;
    bool leaf;

    /// TMP
    sf::RectangleShape shape;
  };
}  // namespace rako
