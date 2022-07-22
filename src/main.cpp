#include "tyracraft_game.hpp"
#include "engine.hpp"

int main() {
  Tyra::Engine engine;
  TyraCraft::TyraCraftGame game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}