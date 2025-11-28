#include "tyracraft_game.hpp"
#include "engine.hpp"

int main(int argc, char* argv[]) {
  Tyra::EngineOptions options;
  options.loadUsbDriver = false;
  options.loadMemoryCardDriver = false;
  options.customGraphycsSettings =
      Tyra::RendererSettings(512.0f, 448.0f, 0.1f, 2500.0f);

  Tyra::Engine engine(options);
  TyraCraft::TyraCraftGame game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}