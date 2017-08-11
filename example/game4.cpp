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
    sf::Vector2f pos;
  };
  struct vel {
    sf::Vector2f vel;
  };
  struct accel {
    sf::Vector2f accel;
  };
  struct rect {
    sf::RectangleShape obj;
  };
  struct sprite {
    sf::Sprite obj;
  };
}

using plyr = meta::list<comp::pos, comp::vel, comp::accel, comp::rect>;
using part = meta::list<comp::pos, comp::vel, comp::accel, comp::sprite>;

using egm_t = entity_group_manager<plyr, part>;

struct game {

  game()
    : win(sf::VideoMode(1200, 1200), "rako", sf::Style::Titlebar | sf::Style::Close)
    , font()
    , stats_text()
    , stats_time()
    , stats_frames(0)
    , texture()
    , stats_fps(std::make_tuple(0, 10000, 0))
    , qt(0, 0, 1200, 1200) {
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
      em.add(comp::pos{{100.f, 100.f}}, comp::vel{{0.f, 0.f}}, comp::accel{{0.f, 0.f}}, r);
  }

  void make_particles() {
    for (int i = 0; i < 5000; ++i) {
      float x = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;
      float y = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;

      comp::sprite sp;
      sp.obj.setTexture(texture);
      em.add(comp::pos{{x * 400 + 400, y * 300 + 300}}, comp::vel{{x * 100.f, y * 100.f}},
             comp::accel{{0, 0}}, sp);
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
    sf::Vector2f a = {0, 0};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) a.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) a.x += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) a.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) a.y += 1.f;
    constexpr const auto sqrt2 = 1.41421356237f;
    auto vel = a * 40.f;
    if (a.x != 0.f && a.y != 0.f) vel /= sqrt2;
    em.get<comp::vel>(player_handle).vel = vel;

    auto const ts = t.asSeconds();

    em.for_each<meta::list<comp::pos, comp::vel, comp::accel>>([ts](auto& p, auto& v, auto& a) {
      v.vel += a.accel * ts;
      p.pos += v.vel * ts;
    });

    auto const ws = win.getSize();
    auto const radius = 14.f;
    em.for_each<meta::list<comp::pos, comp::vel>>([ws, radius](auto const& p, auto& v) {
      if (p.pos.x <= 0) v.vel.x *= -1;
      if (p.pos.x + radius >= ws.x) v.vel.x *= -1;
      if (p.pos.y <= 0) v.vel.y *= -1;
      if (p.pos.y + radius >= ws.y) v.vel.y *= -1;
    });

    em.for_each<meta::list<comp::pos, comp::rect>>(
      [](auto const& p, auto& o) { o.obj.setPosition(p.pos); });
    em.for_each<meta::list<comp::pos, comp::sprite>>(
      [](auto const& p, auto& o) { o.obj.setPosition(p.pos); });

    em.reclaim();

    tcont.clear();  ////
    qt.clear();

    em.for_each<meta::list<egm_t::handle, comp::pos>>(
      [this /*, ts*/](auto h, auto const& p) { qt.insert(h, p.pos.x, p.pos.y, tcont); });

    collnum = 0;
    tcont.test([this, ts](auto const& nodes) {
      for (auto i = 0u; i < nodes.size(); ++i) {
        for (auto j = i + 1; j < nodes.size(); ++j) {
          auto const& x1 = nodes[i].x;
          auto const& y1 = nodes[i].y;
          auto const& x2 = nodes[j].x;
          auto const& y2 = nodes[j].y;
          if (x1 < x2 + 14.f && x1 + 14.f > x2 && y1 < y2 + 14.f && y1 + 14.f > y2) {
            auto const& h1 = nodes[i].obj;
            auto const& h2 = nodes[j].obj;

            auto[p1, v1] = em.get<comp::pos, comp::vel>(h1);
            auto[p2, v2] = em.get<comp::pos, comp::vel>(h2);

            auto const p21x = p2.pos.x - p1.pos.x;
            auto const p21y = p2.pos.y - p1.pos.y;
            auto const v21x = v2.vel.x - v1.vel.x;
            auto const v21y = v2.vel.y - v1.vel.y;
            auto const m = p21x * v21x + p21y * v21y;
            if (m >= 0.f) continue;

            auto const p12x = -1.f * p21x;
            auto const p12y = -1.f * p21y;
            auto const d1 = p12x * p12x + p12y * p12y;
            auto const d2 = p21x * p21x + p21y * p21y;
            auto const v1o = v1.vel;
            auto const v2o = v2.vel;
            v1.vel.x = v1o.x - m / d1 * p12x;
            v1.vel.y = v1o.y - m / d1 * p12y;
            v2.vel.x = v2o.x - m / d2 * p21x;
            v2.vel.y = v2o.y - m / d2 * p21y;

            p1.pos += v1.vel * ts;
            p2.pos += v2.vel * ts;
            ++collnum;
          }
        }
      }
    });
  }

  void render() {
    win.clear();

    //qt.draw(win);

    em.for_each<meta::list<comp::sprite>>([this](auto const& o) { win.draw(o.obj); });

    //em.for_each<meta::list<comp::rect>>([this](auto const& o) { win.draw(o.obj); });

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

  using qtree_t = qtree<egm_t::handle, 256, 8>;
  qtree_t qt;
  tmpcont<qtree_t*> tcont;
  int collnum = 0;
};

int main() {
  game g;
  g.run();

  return 0;
}
