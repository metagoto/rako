#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cassert>
#include <vector>

namespace rako {

  template <typename T, std::size_t ItemsPerNode, std::size_t MaxLevel>
  struct quadtree_node {
    using value_type = T;
    using self_t = quadtree_node<value_type, ItemsPerNode, MaxLevel>;

    struct data {  // short POD data
      value_type obj;
      float x, y;
    };

    static constexpr auto NW = 0;
    static constexpr auto NE = 1;
    static constexpr auto SE = 2;
    static constexpr auto SW = 3;

    quadtree_node(float x, float y, float w, float h, int level = 0)
      : ax(x)
      , ay(y)
      , aw(w)
      , ah(h)
      , level(level)
      , leaf(true) {}

    template <typename U>
    bool insert(value_type v, float x, float y, U& active_nodes) {
      if (x < ax || x > ax + aw || y < ay || y > ay + ah) return false;
      if ((items.size() < ItemsPerNode && leaf) || level == MaxLevel) {
        items.push_back(data{std::move(v), x, y});
        if (items.size() == 2) active_nodes.insert(this);  // == 2 right after 2nd insertion
        return true;
      }
      leaf = false;
      active_nodes.remove(this);

      if (!quad[0]) split();
      std::for_each(std::begin(items), std::end(items), [this, &active_nodes](auto data) {
        (quad[NW]->insert(std::move(data.obj), data.x, data.y, active_nodes) ||
         quad[NE]->insert(std::move(data.obj), data.x, data.y, active_nodes) ||
         quad[SE]->insert(std::move(data.obj), data.x, data.y, active_nodes) ||
         quad[SW]->insert(std::move(data.obj), data.x, data.y, active_nodes));
      });

      items.clear();
      if (quad[NW]->insert(std::move(v), x, y, active_nodes)) return true;
      if (quad[NE]->insert(std::move(v), x, y, active_nodes)) return true;
      if (quad[SE]->insert(std::move(v), x, y, active_nodes)) return true;
      if (quad[SW]->insert(std::move(v), x, y, active_nodes)) return true;
      assert(false);
      return false;
    }

    void split() {
      quad[NW].reset(new self_t(ax, ay, aw / 2.f, ah / 2.f, level + 1));
      quad[NE].reset(new self_t(ax + aw / 2.f, ay, aw / 2.f, ah / 2.f, level + 1));
      quad[SE].reset(new self_t(ax + aw / 2.f, ay + ah / 2.f, aw / 2.f, ah / 2.f, level + 1));
      quad[SW].reset(new self_t(ax, ay + ah / 2.f, aw / 2.f, ah / 2.f, level + 1));
    }

    void clear() {
      if (quad[0]) {
        quad[NW]->clear();
        quad[NE]->clear();
        quad[SE]->clear();
        quad[SW]->clear();
      }
      items.clear();
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
      items.clear();
      leaf = true;
    }

    std::array<std::unique_ptr<self_t>, 4> quad;
    std::vector<data> items;
    float ax, ay, aw, ah;
    int level;
    bool leaf;
  };

  template <typename T, std::size_t ItemsPerNode, std::size_t MaxLevel>
  struct quadtree {
    using value_type = T;
    using qtree_node_t = quadtree_node<T, ItemsPerNode, MaxLevel>;

    template <typename P>
    struct node_set {
      using storage_t = std::vector<P>;
      storage_t nodes;

      auto find(P t) const { return std::find(nodes.begin(), nodes.end(), t); }
      void insert(P t) {
        if (auto i = find(t); i == nodes.end()) nodes.push_back(t);
      }
      void remove(P t) {
        if (auto i = find(t); i != nodes.end()) nodes.erase(i);
      }
      void clear() { nodes.clear(); }
      auto size() const { return nodes.size(); }
      template <typename F>
      auto for_each(F&& f) const {
        for (auto const& p : nodes) { std::forward<F>(f)(p->items); }
      }
    };

    quadtree(float x, float y, float w, float h)
      : root(x, y, w, h) {}
    void insert(value_type v, float x, float y) { root.insert(v, x, y, active_nodes); }
    template <typename F>
    auto for_each(F&& f) const {
      return active_nodes.for_each(std::forward<F>(f));
    }
    auto clear() {
      active_nodes.clear();
      root.clear();
    }
    auto size() const { return active_nodes.size(); }

    // replace with std array?
    node_set<qtree_node_t*> active_nodes;
    qtree_node_t root;
  };
}
