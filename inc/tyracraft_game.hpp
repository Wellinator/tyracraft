#pragma once

#include <engine.hpp>
#include <game.hpp>

namespace TyraCraft {

class TyraCraftGame : public Tyra::Game {
 public:
  TyraCraftGame(Tyra::Engine* engine);
  ~TyraCraftGame();

  void init();
  void loop();

 private:
  Tyra::Engine* engine;
};

}  // namespace TyraCraft
