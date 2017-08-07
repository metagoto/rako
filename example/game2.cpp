#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <rako/entity_group_manager.hpp>

#include <iostream>

using namespace rako;

namespace comp {
  struct body {
    sf::Vector2f pos;
    sf::Vector2f vel;
    sf::Vector2f accel;
  };
  struct rect {
    sf::RectangleShape obj;
  };
  struct sprite {
    sf::Sprite obj;
  };
}  // namespace comp

using plyr = meta::list<comp::body, comp::rect>;
using part = meta::list<comp::body, comp::sprite>;

using egm_t = entity_group_manager<plyr, part>;

struct game {

  game()
    : win(sf::VideoMode(800, 600), "rako", sf::Style::Titlebar | sf::Style::Close)
    , font()
    , stats_text()
    , stats_time()
    , stats_frames(0)
    , texture()
    , stats_fps(std::make_tuple(0, 10000, 0)) {
    win.setKeyRepeatEnabled(false);
    font.loadFromFile("media/Sansation.ttf");
    stats_text.setFont(font);
    stats_text.setPosition(5.f, 5.f);
    stats_text.setCharacterSize(10);

    texture.loadFromFile("media/bullet.png");

    make_player();
    make_particles();
  }

  void make_player() {
    comp::body b = {{100.f, 100.f}, {0.f, 0.f}, {0.f, 0.f}};
    comp::rect r;
    r.obj.setFillColor(sf::Color::Red);
    r.obj.setSize({10.f, 10.f});
    r.obj.setPosition(200.f, 200.f);
    player_handle = em.add(b, r);
  }

  void make_particles() {
    for (int i = 0; i < 1000; ++i) {
      float x = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;
      float y = static_cast<float>(rand() / static_cast<float>(RAND_MAX)) - 0.5f;
      comp::body b = {{x * 400 + 400, y * 300 + 300}, {x * 80.f, y * 80.f}, {0, 0}};
      // comp::body b = {{x * 400 + 400, y * 300 + 300}, {0.f, 0.f}, {0., 0.}};

      comp::sprite sp;
      sp.obj.setTexture(texture);
      em.add(b, sp);
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
    if (event.type == sf::Event::KeyPressed) {}
  }

  void update(sf::Time t) {

    ///
    sf::Vector2f a = {0, 0};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) a.x -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) a.x += 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) a.y -= 1.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) a.y += 1.f;

    constexpr const auto sqrt2 = 1.41421356237f;  // sqrt(2.f);
    auto vel = a * 40.f;
    if (a.x != 0.f && a.y != 0.f) vel /= sqrt2;
    em.get<comp::body>(player_handle).vel = vel;

    auto const ts = t.asSeconds();

    em.for_each<meta::list<comp::body>>([ts](auto& b) {
      b.vel += b.accel * ts;
      b.pos += b.vel * ts;
    });

    ///
    auto const ws = win.getSize();
    auto const radius = 16.f;
    em.for_each<meta::list<comp::body>>([ws, radius](auto& b) {
      if (b.pos.x <= 0) b.vel.x *= -1;
      if (b.pos.x + radius >= ws.x) b.vel.x *= -1;
      if (b.pos.y <= 0) b.vel.y *= -1;
      if (b.pos.y + radius >= ws.y) b.vel.y *= -1;
    });

    ///
    em.for_each<part>([](auto const& b, auto& o) { o.obj.setPosition(b.pos); });
    em.for_each<plyr>([](auto const& b, auto& o) { o.obj.setPosition(b.pos); });
  }

  void render() {
    win.clear();
    em.for_each<meta::list<comp::sprite>>([this](auto const& o) { win.draw(o.obj); });
    em.for_each<meta::list<comp::rect>>([this](auto const& o) { win.draw(o.obj); });

    win.draw(stats_text);
    win.display();
  }

  void update_stats(sf::Time elapsed_time) {
    stats_time += elapsed_time;
    stats_frames += 1;
    if (stats_time >= sf::seconds(1.0f)) {
      stats_text.setString("fps: " + std::to_string(stats_frames) + "\n" + "time / update: " +
                           std::to_string(stats_time.asMicroseconds() / stats_frames) + "us");

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
  // em_t em;
  egm_t em;
  sf::Texture texture;
  std::tuple<std::size_t, std::size_t, std::size_t> stats_fps;  // av, min, max
  egm_t::handle player_handle;
};

int main() {
  game g;
  g.run();

  return 0;
}
