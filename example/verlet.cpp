#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif

#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <rako/entity_group_manager.hpp>
#include "./quadtree.hpp"

using namespace rako;

namespace comp {
  struct pos {
    float x, y;
  };
  struct old_pos {
    float x, y;
  };
  struct accel {
    float x, y;
  };
  struct rect {
    sf::RectangleShape obj;
  };
  struct sprite {
    sf::Sprite obj;
  };
}

using plyr = meta::list<comp::pos, comp::old_pos, comp::accel, comp::rect>;
using part = meta::list<comp::pos, comp::old_pos, comp::accel, comp::sprite>;

using egm_t = entity_group_manager<plyr, part>;

struct game {

  game()
    : win(sf::VideoMode(800, 600), "rako", sf::Style::Titlebar | sf::Style::Close)
    , font()
    , stats_text()
    , stats_time()
    , stats_frames(0)
    , texture()
    , stats_fps(std::make_tuple(0, 10000, 0))
    , qt(0, 0, 800, 600) {
    win.setKeyRepeatEnabled(false);
    font.loadFromFile("media/Sansation.ttf");
    stats_text.setFont(font);
    stats_text.setPosition(5.f, 5.f);
    stats_text.setCharacterSize(12);

    texture.loadFromFile("media/bullet.png");

    make_player();
    make_particles();
  }

  void make_player() {
    comp::rect r;
    r.obj.setFillColor(sf::Color::Red);
    r.obj.setSize({10.f, 10.f});
    r.obj.setPosition(200.f, 200.f);
    player_handle =
      em.add(comp::pos{100.f, 100.f}, comp::old_pos{0.f, 0.f}, comp::accel{0.f, 0.f}, r);
  }

  void make_particles() {
    for (int i = 0; i < 150; ++i) {
      float x = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;
      float y = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;

      comp::sprite sp;
      sp.obj.setTexture(texture);
      em.add(comp::pos{x * 400 + 400, y * 300 + 300},
             comp::old_pos{x * 400 + 400 - 1, y * 300 + 300 - 1}, comp::accel{2.f, 2.f}, sp);
    }
  }

  void run() {
    static auto tpf = sf::seconds(1.f / 60.f);
    sf::Clock clock;
    sf::Time time_since_last_update = sf::Time::Zero;
    while (win.isOpen()) {
      sf::Time elapsed_time = clock.restart();
      time_since_last_update += elapsed_time;
      while (time_since_last_update > tpf) {
        time_since_last_update -= tpf;
        process_input();
        update(tpf);
      }
      update_stats(elapsed_time);
      render();
    }
  }

  void process_input() {
    sf::Event event;
    while (win.pollEvent(event)) {
      handle_event(event);
      if (event.type == sf::Event::Closed) win.close();
    }
  }

  void handle_event(sf::Event event) {
    if (event.type == sf::Event::KeyPressed) {
      if (event.key.code == sf::Keyboard::Escape) { win.close(); }
    }
  }

  void update(sf::Time t) {
    ///
    //    sf::Vector2f a = {0, 0};
    //    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) a.x -= 1.f;
    //    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) a.x += 1.f;
    //    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) a.y -= 1.f;
    //    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) a.y += 1.f;
    //    constexpr const auto sqrt2 = 1.41421356237f;
    //    auto vel = a * 40.f;
    //    if (a.x != 0.f && a.y != 0.f) vel /= sqrt2;
    //    em.get<comp::vel>(player_handle).vel = vel;

    auto const ts = t.asSeconds();

    em.for_each<meta::list<comp::pos, comp::old_pos, comp::accel>>([ts](auto& p, auto& o, auto& a) {
      auto x = p.x;
      auto y = p.y;
      p.x += p.x - o.x + a.x * ts * ts;
      p.y += p.y - o.y + a.y * ts * ts;
      o.x = x;
      o.y = y;
    });

    auto const ws = win.getSize();
    auto const radius = 14.f;
    em.for_each<meta::list<comp::pos, comp::old_pos, comp::accel>>(
      [ws, radius](auto const& p, auto& o, auto& a) {
        if (p.x <= 0) {
          o.x = 2 * p.x - o.x;
          o.y = 2 * p.y - o.y;
        }
        if (p.x + radius >= ws.x) {
          o.x = 2 * p.x - o.x;
          o.y = 2 * p.y - o.y;
        }
        if (p.y <= 0) {
          o.x = 2 * p.x - o.x;
          o.y = 2 * p.y - o.y;
        }
        if (p.y + radius >= ws.y) {
          // o.x = 2 * p.x - o.x;
          // o.y = 2 * p.y - o.y;
          a.x = 0;
          a.y = -50;
        }
      });

    //    em.for_each<meta::list<egm_t::handle, comp::pos, comp::vel>>(
    //      [this, ts](auto h1, auto& p, auto& v)
    //      {
    //        em.for_each<meta::list<egm_t::handle, comp::pos, comp::vel>>([&](auto h2, auto& q,
    //        auto& w)
    //          {
    //            if (h1 == h2) return;
    //            sf::Rect<float> r1 = {p.pos.x, p.pos.y, 14, 14};
    //            sf::Rect<float> r2 = {q.pos.x, q.pos.y, 14, 14};
    //            if (r1.intersects(r2)) {
    //              v.vel *= -1.f;
    //              w.vel *= -1.f;
    //              p.pos += v.vel * ts;
    //              q.pos += w.vel * ts;
    //            }
    //          });
    //      });

    em.for_each<meta::list<comp::pos, comp::rect>>(
      [](auto const& p, auto& o) { o.obj.setPosition(p.x, p.y); });
    em.for_each<meta::list<comp::pos, comp::sprite>>(
      [](auto const& p, auto& o) { o.obj.setPosition(p.x, p.y); });

    em.reclaim();

    //    tcont.clear(); ////
    //    qt.clear();

    //    em.for_each<meta::list<egm_t::handle, comp::pos>>([this, ts](auto h1, auto const& p)
    //      {
    //        qt.insert(h1, p.pos.x, p.pos.y, tcont);
    //      });

    //    collnum = 0;
    //    tcont.test([this, ts](auto const& nodes) // data.x, data.y!!!
    //      {
    //        for (auto i = 0u; i < nodes.size(); ++i) {
    //          for (auto j = i + 1; j < nodes.size(); ++j) {
    //            auto const& x1 = nodes[i].x; // no handle for now
    //            auto const& y1 = nodes[i].y;
    //            auto const& x2 = nodes[j].x;
    //            auto const& y2 = nodes[j].y;
    //            if (x1 < x2 + 14.f && x1 + 14.f > x2 && y1 < y2 + 14.f && y1 + 14.f > y2) {
    //              ++collnum;

    //              auto const& h1 = nodes[i].obj;
    //              auto const& h2 = nodes[j].obj;

    //              auto& p1 = em.get<comp::pos>(h1);
    //              auto& v1 = em.get<comp::vel>(h1);
    //              auto& p2 = em.get<comp::pos>(h2);
    //              auto& v2 = em.get<comp::vel>(h2);

    //              v1.vel *= -1.f;
    //              v2.vel *= -1.f;
    //              p1.pos += v1.vel * ts;
    //              p2.pos += v2.vel * ts;
    //            }
    //          }
    //        }
    //      });
  }

  void render() {
    win.clear();

    // qt.draw(win);

    em.for_each<meta::list<comp::sprite>>([this](auto const& o) { win.draw(o.obj); });
    em.for_each<meta::list<comp::rect>>([this](auto const& o) { win.draw(o.obj); });

    win.draw(stats_text);
    win.display();
  }

  void update_stats(sf::Time elapsed_time) {
    using std::to_string;

    stats_time += elapsed_time;
    stats_frames += 1;
    if (stats_time >= sf::seconds(1.0f)) {
      stats_text.setString("fps: " + to_string(stats_frames) + "\n" + "time / update: " +
                           to_string(stats_time.asMicroseconds() / stats_frames) + "us\n" +
                           "#entities: " + to_string(em.size()) + "\n" +
                           "#tcont: " + to_string(tcont.size()) + " #coll: " + to_string(collnum));

      std::get<0>(stats_fps) = (std::get<0>(stats_fps) + stats_frames) / 2;
      if (std::get<1>(stats_fps) > stats_frames) std::get<1>(stats_fps) = stats_frames;
      if (std::get<2>(stats_fps) < stats_frames) std::get<2>(stats_fps) = stats_frames;

      stats_time -= sf::seconds(1.0f);
      stats_frames = 0;
    }
  }

  ~game() {
    std::cout << "stats: " << std::get<0>(stats_fps) << " " << std::get<1>(stats_fps) << " "
              << std::get<2>(stats_fps) << " " << std::endl;
  }

  sf::RenderWindow win;
  sf::Font font;
  sf::Text stats_text;
  sf::Time stats_time;
  std::size_t stats_frames;
  egm_t em;
  sf::Texture texture;
  std::tuple<std::size_t, std::size_t, std::size_t> stats_fps;  // av, min, max
  egm_t::handle player_handle;

  using qtree_t = qtree<egm_t::handle, 4, 8>;
  qtree_t qt;
  tmpcont<qtree_t*> tcont;
  int collnum = 0;
};

int main() {
  game g;
  g.run();

  return 0;
}
