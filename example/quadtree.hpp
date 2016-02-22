#pragma once

#include <cassert>
#include <vector>
#include <SFML/Graphics.hpp>


namespace rako
{

  template <typename T, int MaxNode, int MaxLevel>
  struct qtree
  {
    using self_t = qtree<T, MaxNode, MaxLevel>;
    using value_type = T;

    struct data
    {
      T obj;
      float x, y;
    };

    static constexpr auto NW = 0;
    static constexpr auto NE = 1;
    static constexpr auto SE = 2;
    static constexpr auto SW = 3;

    qtree(float x, float y, float w, float h, int level)
      : ax(x)
      , ay(y)
      , aw(w)
      , ah(h)
      , level(level)
      , num_node(0)
      , shape(sf::Vector2f(w, h))
    {
      shape.setPosition(x, y);
      shape.setOutlineThickness(1.f);
      shape.setOutlineColor(sf::Color::Red);
      shape.setFillColor(sf::Color::Black);
    }

    bool insert(value_type v, float x, float y) // also const&
    {
      if (x < ax || x > ax + aw || y < ay || y > ay + ah) return false;
      if ((num_node < MaxNode && !quad[0]) || level == MaxLevel) {
        ++num_node;
        nodes.push_back(data{std::move(v), x, y}); //
        return true;
      }
      if (!quad[0]) {
        quad[NW].reset(new self_t(ax, ay, aw / 2.f, ah / 2.f, level + 1));
        quad[NE].reset(new self_t(ax + aw / 2.f, ay, aw / 2.f, ah / 2.f, level + 1));
        quad[SE].reset(new self_t(ax + aw / 2.f, ay + ah / 2.f, aw / 2.f, ah / 2.f, level + 1));
        quad[SW].reset(new self_t(ax, ay + ah / 2.f, aw / 2.f, ah / 2.f, level + 1));
        std::for_each(std::begin(nodes), std::end(nodes), [this](auto data)
          {
            insert(std::move(data.obj), data.x, data.y);
          });
        nodes.clear();
        num_node = 0;
      }
      if (quad[NW]->insert(std::move(v), x, y)) return true;
      if (quad[NE]->insert(std::move(v), x, y)) return true;
      if (quad[SE]->insert(std::move(v), x, y)) return true;
      if (quad[SW]->insert(std::move(v), x, y)) return true;
      assert(false);
      return false;
    }

    void clear()
    {
      if (quad[NW]) {
        quad[NW].reset();
        quad[NE].reset();
        quad[SE].reset();
        quad[SW].reset();
      }
      nodes.clear();
      num_node = 0;
    }

    void draw(sf::RenderTarget& target)
    {
      target.draw(shape);
      if (quad[NW]) {
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
    int num_node;

    ///TMP
    sf::RectangleShape shape;
  };
}
