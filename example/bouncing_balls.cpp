#ifdef __clang__  // silence these SFML warnings (-Werror)
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <rako/entity_manager.hpp>
#include "./quadtree.hpp"

#include <X11/Xlib.h>
#include <thread>

using namespace rako;

namespace component {  // our components

  struct position {
    sf::Vector2f pos;
  };
  struct velocity {
    sf::Vector2f vel;
  };
  struct acceleration {
    sf::Vector2f accel;
  };
  struct rect {
    sf::RectangleShape obj;
  };
  struct sprite {
    sf::Sprite obj;
  };
}

namespace c = component;

// components group for the player entity
using player_components = meta::list<c::position, c::velocity, c::acceleration, c::rect>;

// components group for balls entities
using ball_components = meta::list<c::position, c::velocity, c::acceleration, c::sprite>;

// entity manager type
using manager = entity_manager<player_components, ball_components>;

struct game {

  game(int width, int height, int num_balls = 200)
    : win(sf::VideoMode(width, height), "rako", sf::Style::Titlebar | sf::Style::Close)
    , font()
    , stats_text()
    , stats_time()
    , stats_frames(0)
    , texture()
    , stats_fps(std::make_tuple(0, 10000, 0))
    , qtree(0, 0, width, height) {
    win.setKeyRepeatEnabled(false);
    font.loadFromFile("media/Sansation.ttf");
    stats_text.setFont(font);
    stats_text.setPosition(5.f, 5.f);
    stats_text.setCharacterSize(14);
    texture.loadFromFile("media/bullet.png");
    make_player();
    make_particles(num_balls);
  }

  void make_player() {
    sf::RectangleShape r;
    r.setFillColor(sf::Color::Red);
    r.setSize({14.f, 14.f});
    // create the payer and get back its handle for later use as (a member variable).
    // the manager automatically knows in which group the new entity belongs.
    // parameters order isn't important as long as their types form an entity group
    player_handle = em.add(c::position{{50.f, 50.f}}, c::velocity{{0.f, 0.f}},
                           c::acceleration{{0.f, 0.f}}, c::rect{r});
  }

  void make_particles(int num) {
    for (int i = 0; i < num; ++i) {
      float x = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;
      float y = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;
      c::sprite sp;
      sp.obj.setTexture(texture);
      em.add(c::position{{x * 400 + 400, y * 300 + 300}}, c::velocity{{x * 100.f, y * 100.f}},
             c::acceleration{{0, 0}}, sp);
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
    // update player position
    sf::Vector2f a = {0, 0};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) a.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) a.x += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) a.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) a.y += 1.f;
    constexpr const auto sqrt2 = 1.41421356237f;
    auto vel = a * 80.f;
    if (a.x != 0.f && a.y != 0.f) vel /= sqrt2;
    em.get<c::velocity>(player_handle).vel = vel;

    auto const ts = t.asSeconds();

    // update velocity and position for entities that have
    // position, velocity and accelaration components
    em.for_each<c::position, c::velocity, c::acceleration>([ts](auto& p, auto& v, auto& a) {
      v.vel += a.accel * ts;
      p.pos += v.vel * ts;
    });

    // make entities bounce of the egdes of the window
    auto const ws = win.getSize();
    auto const radius = 14.f;
    em.for_each<c::position, c::velocity>([ws, radius](auto& p, auto& v) {
      if (p.pos.x <= 0) {
        p.pos.x = 0;
        v.vel.x *= -1;
      } else if (p.pos.x + radius >= ws.x) {
        p.pos.x = ws.x - radius;
        v.vel.x *= -1;
      }
      if (p.pos.y <= 0) {
        p.pos.y = 0;
        v.vel.y *= -1;
      } else if (p.pos.y + radius >= ws.y) {
        p.pos.y = ws.y - radius;
        v.vel.y *= -1;
      }
    });

    // put entities handle in a quadtree for efficient collision detection
    qtree.clear();
    em.for_each<manager::handle, c::position>(
      [this](auto h, auto const& p) { qtree.insert(h, p.pos.x, p.pos.y); });

    // update colliding entities position and velocity.
    // loop through quadtree nodes that contains at least 2 items.
    // struct item { handle obj; float x, y; };
    collnum = 0;
    qtree.for_each([this, ts, radius](auto const& items) {
      for (auto i = 0u; i < items.size(); ++i) {
        for (auto j = i + 1; j < items.size(); ++j) {
          auto const x1 = items[i].x;
          auto const y1 = items[i].y;
          auto const x2 = items[j].x;
          auto const y2 = items[j].y;
          if (x1 < x2 + radius && x1 + radius > x2 && y1 < y2 + radius && y1 + radius > y2) {

            // get handles for both colliding entities
            auto const& h1 = items[i].obj;
            auto const& h2 = items[j].obj;

            // structured binding FTW ;)
            // p1..v2 actually are references as per c++ standard
            auto[p1, v1] = em.get<c::position, c::velocity>(h1);
            auto[p2, v2] = em.get<c::position, c::velocity>(h2);

            // compute and update positions and velocities
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

    // update sfml objects
    em.for_each<c::position, c::rect>([](auto const& p, auto& o) { o.obj.setPosition(p.pos); });
    em.for_each<c::position, c::sprite>([](auto const& p, auto& o) { o.obj.setPosition(p.pos); });
  }

  void run(bool use_render_thread = false) {
    if (use_render_thread) return run_thread();
    auto const tpf = sf::seconds(1.f / 60.f);
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

  void render() {
    win.clear();
    // draw balls
    em.for_each<c::sprite>([this](auto const& o) { win.draw(o.obj); });
    // draw player
    win.draw(em.get<c::rect>(player_handle).obj);
    // draw stats
    win.draw(stats_text);
    win.display();
  }

  void run_thread() {  // very naive. dont do it that way in prod ;)
    auto const tpf = sf::seconds(1.f / 60.f);
    sf::Clock clock;
    sf::Time time_since_last_update = sf::Time::Zero;
    win.setActive(false);
    std::thread th(&game::render_thread, this);
    while (win.isOpen()) {
      sf::Time elapsed_time = clock.restart();
      time_since_last_update += elapsed_time;
      while (time_since_last_update > tpf) {
        time_since_last_update -= tpf;
        process_input();
        update(tpf);
      }
    }
    th.join();
  }

  void render_thread() {
    sf::Clock clock;
    sf::Time time_since_last_update = sf::Time::Zero;
    while (win.isOpen()) {
      sf::Time elapsed_time = clock.restart();
      time_since_last_update += elapsed_time;
      update_stats(elapsed_time);
      render();
    }
  }

  // some stats displayed on screen
  void update_stats(sf::Time elapsed_time) {
    using std::to_string;
    stats_time += elapsed_time;
    stats_frames += 1;
    if (stats_time >= sf::seconds(1.0f)) {
      stats_text.setString("fps: " + to_string(stats_frames) + "\n" + "time / update: " +
                           to_string(stats_time.asMicroseconds() / stats_frames) + "us\n" +
                           "#entities: " + to_string(em.size()) + "\n" + "#active qtree leaf: " +
                           to_string(qtree.size()) + " #collisions: " + to_string(collnum));

      std::get<0>(stats_fps) = (std::get<0>(stats_fps) + stats_frames) / 2;
      if (std::get<1>(stats_fps) > stats_frames) std::get<1>(stats_fps) = stats_frames;
      if (std::get<2>(stats_fps) < stats_frames) std::get<2>(stats_fps) = stats_frames;
      stats_time -= sf::seconds(1.0f);
      stats_frames = 0;
    }
  }

  ~game() {
    std::cout << "fps (av, min, max): " << std::get<0>(stats_fps) << ", " << std::get<1>(stats_fps)
              << ", " << std::get<2>(stats_fps) << " " << std::endl;
  }

  sf::RenderWindow win;
  sf::Font font;
  sf::Text stats_text;
  sf::Time stats_time;
  std::size_t stats_frames;
  manager em;
  sf::Texture texture;
  std::tuple<std::size_t, std::size_t, std::size_t> stats_fps;  // av, min, max
  manager::handle player_handle;

  using quadtree_t = quadtree<manager::handle, 256, 8>;
  quadtree_t qtree;
  int collnum = 0;
};

int main() {
  XInitThreads();
  game g(800, 600, 300);  // 800x600 window, N bouncing balls
  g.run(false);           // run(true) for a dedicated render thread
  return 0;
}
